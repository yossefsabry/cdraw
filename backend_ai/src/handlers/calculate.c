#include "calculate.h"
#include "../config.h"
#include "../utils/http_client.h"
#include "../utils/json.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char expr[128];
  char result[128];
  int assign;
} CalcItem;

static void RespondError(HttpResponse *res, int status, const char *msg) {
  if (!res)
    return;
  res->status = status;
  res->content_type = "application/json";
  char *esc = JsonEscape(msg ? msg : "error");
  if (!esc)
    esc = strdup("error");
  size_t sz = strlen(esc) + 64;
  res->body = (char *)malloc(sz);
  if (res->body)
    snprintf(res->body, sz,
             "{\"status\":\"error\",\"message\":\"%s\"}", esc);
  free(esc);
}

static char *JsonExtractStringAlloc(const char *json, const char *key) {
  char *raw = JsonExtractRaw(json, key);
  if (!raw)
    return NULL;
  size_t len = strlen(raw);
  if (len >= 2 &&
      ((raw[0] == '"' && raw[len - 1] == '"') ||
       (raw[0] == '\'' && raw[len - 1] == '\''))) {
    raw[len - 1] = '\0';
    char *out = strdup(raw + 1);
    free(raw);
    return out;
  }
  return raw;
}

static const char *StripImagePrefix(const char *image) {
  if (!image)
    return NULL;
  const char *p = strstr(image, "base64,");
  if (p)
    return p + 7;
  return image;
}

static char *BuildPrompt(const char *dict_raw) {
  const char *dict = dict_raw && dict_raw[0] ? dict_raw : "{}";
  const char *fmt =
      "You have been given an image with some mathematical expressions, equations, or graphical problems, and you need to solve them. "
      "Note: Use the PEMDAS rule for solving mathematical expressions. PEMDAS stands for the Priority Order: Parentheses, Exponents, Multiplication and Division (from left to right), Addition and Subtraction (from left to right). Parentheses have the highest priority, followed by Exponents, then Multiplication and Division, and lastly Addition and Subtraction. "
      "For example: "
      "Q. 2 + 3 * 4 "
      "(3 * 4) => 12, 2 + 12 = 14. "
      "Q. 2 + 3 + 5 * 4 - 8 / 2 "
      "5 * 4 => 20, 8 / 2 => 4, 2 + 3 => 5, 5 + 20 => 25, 25 - 4 => 21. "
      "YOU CAN HAVE FIVE TYPES OF EQUATIONS/EXPRESSIONS IN THIS IMAGE, AND ONLY ONE CASE SHALL APPLY EVERY TIME: "
      "Following are the cases: "
      "1. Simple mathematical expressions like 2 + 2, 3 * 4, 5 / 6, 7 - 8, etc.: In this case, solve and return the answer in the format of a LIST OF ONE DICT [{'expr': given expression, 'result': calculated answer}]. "
      "2. Set of Equations like x^2 + 2x + 1 = 0, 3y + 4x = 0, 5x^2 + 6y + 7 = 12, etc.: In this case, solve for the given variable, and the format should be a COMMA SEPARATED LIST OF DICTS, with dict 1 as {'expr': 'x', 'result': 2, 'assign': True} and dict 2 as {'expr': 'y', 'result': 5, 'assign': True}. This example assumes x was calculated as 2, and y as 5. Include as many dicts as there are variables. "
      "3. Assigning values to variables like x = 4, y = 5, z = 6, etc.: In this case, assign values to variables and return another key in the dict called {'assign': True}, keeping the variable as 'expr' and the value as 'result' in the original dictionary. RETURN AS A LIST OF DICTS. "
      "4. Analyzing Graphical Math problems, which are word problems represented in drawing form, such as cars colliding, trigonometric problems, problems on the Pythagorean theorem, adding runs from a cricket wagon wheel, etc. These will have a drawing representing some scenario and accompanying information with the image. PAY CLOSE ATTENTION TO DIFFERENT COLORS FOR THESE PROBLEMS. You need to return the answer in the format of a LIST OF ONE DICT [{'expr': given expression, 'result': calculated answer}]. "
      "5. Detecting Abstract Concepts that a drawing might show, such as love, hate, jealousy, patriotism, or a historic reference to war, invention, discovery, quote, etc. USE THE SAME FORMAT AS OTHERS TO RETURN THE ANSWER, where 'expr' will be the explanation of the drawing, and 'result' will be the abstract concept. "
      "Analyze the equation or expression in this image and return the answer according to the given rules: "
      "Make sure to use extra backslashes for escape characters like \\f -> \\\\f, \\n -> \\\\n, etc. "
      "Here is a dictionary of user-assigned variables. If the given expression has any of these variables, use its actual value from this dictionary accordingly: %s. "
      "DO NOT USE BACKTICKS OR MARKDOWN FORMATTING. "
      "PROPERLY QUOTE THE KEYS AND VALUES IN THE DICTIONARY FOR EASIER PARSING WITH Python's ast.literal_eval.";

  size_t sz = strlen(fmt) + strlen(dict) + 32;
  char *out = (char *)malloc(sz);
  if (!out)
    return NULL;
  snprintf(out, sz, fmt, dict);
  return out;
}

static void TrimSpaces(char *s) {
  if (!s)
    return;
  size_t len = strlen(s);
  size_t start = 0;
  while (start < len && isspace((unsigned char)s[start]))
    start++;
  size_t end = len;
  while (end > start && isspace((unsigned char)s[end - 1]))
    end--;
  if (start > 0)
    memmove(s, s + start, end - start);
  s[end - start] = '\0';
}

static bool ExtractValue(const char *dict, const char *key, char *out, size_t out_sz) {
  if (!dict || !key || !out || out_sz == 0)
    return false;
  out[0] = '\0';
  const char *p = dict;
  size_t key_len = strlen(key);
  while ((p = strstr(p, key)) != NULL) {
    const char *kstart = p;
    if (kstart > dict && (isalnum((unsigned char)kstart[-1]) || kstart[-1] == '_')) {
      p += key_len;
      continue;
    }
    p += key_len;
    while (*p && *p != ':')
      p++;
    if (*p != ':')
      continue;
    p++;
    while (*p && isspace((unsigned char)*p))
      p++;
    if (*p == '\0')
      return false;
    if (*p == '"' || *p == '\'') {
      char quote = *p++;
      size_t j = 0;
      while (*p && *p != quote && j + 1 < out_sz) {
        if (*p == '\\' && p[1])
          p++;
        out[j++] = *p++;
      }
      out[j] = '\0';
      TrimSpaces(out);
      return j > 0;
    }
    size_t j = 0;
    while (*p && *p != ',' && *p != '}' && j + 1 < out_sz) {
      out[j++] = *p++;
    }
    out[j] = '\0';
    TrimSpaces(out);
    return j > 0;
  }
  return false;
}

static int ParseCalcItems(const char *text, CalcItem *items, int max_items) {
  if (!text || !items || max_items <= 0)
    return 0;
  int count = 0;
  const char *p = text;
  while ((p = strchr(p, '{')) != NULL && count < max_items) {
    const char *end = strchr(p, '}');
    if (!end)
      break;
    size_t len = (size_t)(end - p + 1);
    if (len >= 512) {
      p = end + 1;
      continue;
    }
    char dict[512];
    memcpy(dict, p, len);
    dict[len] = '\0';

    CalcItem item;
    memset(&item, 0, sizeof(item));
    if (!ExtractValue(dict, "expr", item.expr, sizeof(item.expr))) {
      p = end + 1;
      continue;
    }
    if (!ExtractValue(dict, "result", item.result, sizeof(item.result))) {
      p = end + 1;
      continue;
    }
    char assign[16] = {0};
    if (ExtractValue(dict, "assign", assign, sizeof(assign))) {
      if (strcmp(assign, "True") == 0 || strcmp(assign, "true") == 0 || strcmp(assign, "1") == 0)
        item.assign = 1;
    }
    items[count++] = item;
    p = end + 1;
  }
  return count;
}

void HandleCalculate(const HttpRequest *req, HttpResponse *res) {
  if (!req || !res)
    return;
  if (!req->body || req->body_len == 0) {
    RespondError(res, 400, "missing body");
    return;
  }

  char *api_key = JsonExtractStringAlloc(req->body, "api_key");
  if (!api_key || api_key[0] == '\0') {
    free(api_key);
    RespondError(res, 401, "Missing apiKey");
    return;
  }

  char *image = JsonExtractStringAlloc(req->body, "image");
  if (!image) {
    free(api_key);
    RespondError(res, 400, "missing image");
    return;
  }
  char *dict_raw = JsonExtractRaw(req->body, "dict_of_vars");
  char *prompt = BuildPrompt(dict_raw);
  if (!prompt) {
    free(image);
    free(dict_raw);
    free(api_key);
    RespondError(res, 500, "prompt failed");
    return;
  }
  char *prompt_esc = JsonEscape(prompt);
  if (!prompt_esc) {
    free(image);
    free(dict_raw);
    free(prompt);
    free(api_key);
    RespondError(res, 500, "prompt escape failed");
    return;
  }

  const char *b64 = StripImagePrefix(image);
  size_t body_sz = strlen(prompt_esc) + strlen(b64) + 512;
  char *body = (char *)malloc(body_sz);
  if (!body) {
    free(image);
    free(dict_raw);
    free(prompt);
    free(prompt_esc);
    free(api_key);
    RespondError(res, 500, "alloc failed");
    return;
  }
  snprintf(body, body_sz,
           "{\"contents\":[{\"role\":\"user\","
           "\"parts\":[{\"text\":\"%s\"},"
           "{\"inline_data\":{\"mime_type\":\"image/png\",\"data\":\"%s\"}}]}]}",
           prompt_esc, b64);
  char url[512];
  snprintf(url, sizeof(url),
           "https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s",
           DEFAULT_GEMINI_MODEL, api_key);

  char err[128] = {0};
  char *resp = NULL;
  bool ok = HttpPostJson(url, body, NULL, &resp, err, sizeof(err));
  if (!ok && err[0] != '\0' && strncmp(err, "http 404", 8) == 0) {
    err[0] = '\0';
    snprintf(url, sizeof(url),
             "https://generativelanguage.googleapis.com/v1/models/%s:generateContent?key=%s",
             DEFAULT_GEMINI_MODEL, api_key);
    ok = HttpPostJson(url, body, NULL, &resp, err, sizeof(err));
  }

  free(image);
  free(dict_raw);
  free(prompt);
  free(prompt_esc);
  free(body);

  if (!ok) {
    RespondError(res, 502, err[0] ? err : "upstream failed");
    free(resp);
    free(api_key);
    return;
  }

  char text[4096] = {0};
  if (!JsonFindString(resp, "text", text, sizeof(text))) {
    free(resp);
    RespondError(res, 502, "parse failed");
    free(api_key);
    return;
  }

  CalcItem items[64];
  int count = ParseCalcItems(text, items, 64);
  size_t cap = (size_t)(256 + count * 256);
  char *out = (char *)malloc(cap);
  if (!out) {
    free(resp);
    RespondError(res, 500, "alloc failed");
    return;
  }
  size_t off = 0;
  off += snprintf(out + off, cap - off, "{\"status\":\"success\",\"data\":[");
  for (int i = 0; i < count; i++) {
    char *expr_esc = JsonEscape(items[i].expr);
    char *res_esc = JsonEscape(items[i].result);
    if (!expr_esc || !res_esc) {
      free(expr_esc);
      free(res_esc);
      continue;
    }
    off += snprintf(out + off, cap - off,
                    "%s{\"expr\":\"%s\",\"result\":\"%s\",\"assign\":%s}",
                    (i == 0 ? "" : ","),
                    expr_esc, res_esc,
                    items[i].assign ? "true" : "false");
    free(expr_esc);
    free(res_esc);
  }
  off += snprintf(out + off, cap - off, "]}");

  res->status = 200;
  res->content_type = "application/json";
  res->body = out;
  free(resp);
  free(api_key);
}
