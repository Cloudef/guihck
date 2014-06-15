#include "glhckElements.h"
#include "guihckElementUtils.h"
#include "glhck/glhck.h"
#include "lut.h"
#include <stdio.h>

#if defined(_MSC_VER)
# define _GUIHCK_GLHCK_TLS __declspec(thread)
# define _GUIHCK_GLHCK_TLS_FOUND
#elif defined(__GNUC__)
# define _GUIHCK_GLHCK_TLS __thread
# define _GUIHCK_GLHCK_TLS_FOUND
#else
# define _GUIHCK_GLHCK_TLS
# warning "No Thread-local storage! Multi-threaded guihck applications may have unexpected behaviour!"
#endif

static const char GUIHCK_GLHCK_RECTANGLE_SCM[] =
    "(define (rectangle . args)"
    "  (define default-args (list"
    "    (prop 'x 0)"
    "    (prop 'y 0)"
    "    (prop 'width 100)"
    "    (prop 'height 100)"
    "    (prop 'color '(255 255 255))))"
    "  (create-element 'rectangle (append default-args args)))"
    ;

static const char GUIHCK_GLHCK_TEXT_SCM[] =
    "(define (text . args)"
    "  (define default-args (list"
    "    (prop 'x 0)"
    "    (prop 'y 0)"
    "    (prop 'size 12)"
    "    (prop 'text \"\")"
    "    (prop 'font \"\")"
    "    (prop 'color '(255 255 255))))"
    "  (create-element 'text (append default-args args)))"
    ;

static const char GUIHCK_GLHCK_IMAGE_SCM[] =
    "(define (image . args)"
    "  (define default-args (list"
    "    (prop 'x 0)"
    "    (prop 'y 0)"
    "    (prop 'source \"\")"
    "    (prop 'source-width 0)"
    "    (prop 'source-height 0)"
    "    (prop 'color '(255 255 255))))"
    "  (create-element 'image (append default-args args)))"
    ;

static const char GUIHCK_GLHCK_TEXT_INPUT_SCM[] =
    "(define (text-input-gen)"
    "  (define (append-char! c)"
    "    (set-prop! 'text"
    "      (list->string"
    "        (append"
    "          (string->list"
    "            (get-prop 'text)) (list c)))))"
    "  (define (pop-char!)"
    "    (let ((current-text (get-prop 'text)))"
    "      (if (> (string-length current-text) 0)"
    "        (set-prop! 'text (string-drop-right current-text 1)))))"

    "  (define (handle-key! key scancode action mods)"
    "    (cond ((and (= key (keyboard 'backspace))"
    "                (not (eq? action 'release)))"
    "          (pop-char!))))"

    "  (composite item"
    "    (prop 'on-char append-char!)"
    "    (prop 'on-key handle-key!)"
    "    (alias 'text 'text-content 'text)"
    "    (alias 'font 'text-content 'font)"
    "    (alias 'color 'text-content 'color)"
    "    (alias 'size 'text-content 'size)"
    "    (text"
    "      (id 'text-content))"
    "    (rectangle"
    "      (prop 'height (bound '(parent height)))"
    "      (prop 'width 1)"
    "      (alias 'color 'parent 'color)"
    "      (prop 'x (bound '(text-content width)))"
    "      (prop 'visible (bound '(timer value parent focus) (lambda (v f) (and v f)))))"
    "    (timer"
    "      (id 'timer)"
    "      (prop 'interval 0.5)"
    "      (prop 'repeat -1)"
    "      (prop 'running (bound '(parent focus)))"
    "      (prop 'value #t)"
    "      (method 'on-timeout (lambda (c) (set-prop! 'value (not (get-prop 'value))))))"
    "    (mouse-area "
    "      (prop 'width (bound '(parent width)))"
    "      (prop 'height (bound '(parent height)))"
    "      (method 'on-mouse-down (lambda (b x y) (focus! (parent)))))))"

    "(define text-input (text-input-gen))";

static void initRectangle(guihckContext* ctx, guihckElementId id, void* data);
static void destroyRectangle(guihckContext* ctx, guihckElementId id, void* data);
static bool updateRectangle(guihckContext* ctx, guihckElementId id, void* data);
static void renderRectangle(guihckContext* ctx, guihckElementId id, void* data);

typedef struct _guihckGlhckTextContext
{
  glhckHandle text;
  chckHashTable* fonts;
  int textRefs;
} _guihckGlhckTextContext;

static _GUIHCK_GLHCK_TLS _guihckGlhckTextContext textThreadLocalContext = {0, NULL, 0};

typedef struct _guihckGlhckText
{
  glhckFont font;
  glhckHandle object;
  char* content;
  char* fontPath;
} _guihckGlhckText;

static void initText(guihckContext* ctx, guihckElementId id, void* data);
static void destroyText(guihckContext* ctx, guihckElementId id, void* data);
static bool updateText(guihckContext* ctx, guihckElementId id, void* data);
static void renderText(guihckContext* ctx, guihckElementId id, void* data);
static unsigned int getFont(const char* fontPath);

typedef struct _guihckGlhckImage
{
  glhckHandle object;
  char* source;
} _guihckGlhckImage;

static void initImage(guihckContext* ctx, guihckElementId id, void* data);
static void destroyImage(guihckContext* ctx, guihckElementId id, void* data);
static bool updateImage(guihckContext* ctx, guihckElementId id, void* data);
static void renderImage(guihckContext* ctx, guihckElementId id, void* data);

void guihckGlhckAddAllTypes(guihckContext* ctx)
{
  guihckGlhckAddRectangleType(ctx);
  guihckGlhckAddTextType(ctx);
  guihckGlhckAddImageType(ctx);
  guihckGlhckAddTextInputType(ctx);
}

void guihckGlhckAddRectangleType(guihckContext* ctx)
{
  guihckElementTypeFunctionMap functionMap = {
    initRectangle,
    destroyRectangle,
    updateRectangle,
    renderRectangle,
    NULL,
    NULL
  };
  guihckElementTypeAdd(ctx, "rectangle", functionMap, sizeof(glhckHandle));
  guihckContextExecuteScript(ctx, GUIHCK_GLHCK_RECTANGLE_SCM);
}

void initRectangle(guihckContext* ctx, guihckElementId id, void* data)
{
  glhckHandle o = glhckPlaneNew(1.0, 1.0);
  glhckHandle m = glhckMaterialNew(0);
  glhckObjectMaterial(o, m);
  glhckHandleRelease(m);

  memcpy(data, &o, sizeof(glhckHandle));
  guihckElementAddParentPositionListeners(ctx, id);
  guihckElementAddUpdateProperty(ctx, id, "absolute-x");
  guihckElementAddUpdateProperty(ctx, id, "absolute-y");
  guihckElementAddUpdateProperty(ctx, id, "width");
  guihckElementAddUpdateProperty(ctx, id, "height");
  guihckElementAddUpdateProperty(ctx, id, "color");
}

void destroyRectangle(guihckContext* ctx, guihckElementId id, void* data)
{
  (void) ctx;
  (void) id;

  glhckHandleRelease(*(glhckHandle*)data);
}

bool updateRectangle(guihckContext* ctx, guihckElementId id, void* data)
{
  glhckHandle o = *(glhckHandle*)data;
  SCM x = guihckElementGetProperty(ctx, id, "absolute-x");
  SCM y = guihckElementGetProperty(ctx, id, "absolute-y");
  SCM w = guihckElementGetProperty(ctx, id, "width");
  SCM h = guihckElementGetProperty(ctx, id, "height");
  SCM c = guihckElementGetProperty(ctx, id, "color");

  kmVec3 position = *glhckObjectGetPosition(o);
  kmVec3 scale = *glhckObjectGetScale(o);
  if(scm_is_real(w)) scale.x = scm_to_double(w)/2;
  if(scm_is_real(h)) scale.y = scm_to_double(h)/2;
  if(scm_is_real(x)) position.x = scm_to_double(x) + scale.x;
  if(scm_is_real(y)) position.y = scm_to_double(y) + scale.y;

  glhckObjectPosition(o, &position);
  glhckObjectScale(o, &scale);

  if(scm_to_bool(scm_list_p(c)) && scm_to_int32(scm_length(c)) == 3)
  {
    glhckColor color = glhckMaterialGetDiffuse(glhckObjectGetMaterial(o));
    SCM r = scm_list_ref(c, scm_from_int8(0));
    SCM g = scm_list_ref(c, scm_from_int8(1));
    SCM b = scm_list_ref(c, scm_from_int8(2));

    unsigned char cr = (color & 0xFF000000) >> 24;
    unsigned char cg = (color & 0x00FF0000) >> 16;
    unsigned char cb = (color & 0x0000FF00) >> 8;
    unsigned char ca = (color & 0x000000FF);

    if(scm_to_bool(scm_integer_p(r))) cr = scm_to_uint8(r);
    if(scm_to_bool(scm_integer_p(g))) cg = scm_to_uint8(g);
    if(scm_to_bool(scm_integer_p(b))) cb = scm_to_uint8(b);

    glhckColor newColor = (ca | (cb << 8) | (cg << 16) | (cr << 24));
    glhckMaterialDiffuse(glhckObjectGetMaterial(o), newColor);
  }

  return false;
}

void renderRectangle(guihckContext* ctx, guihckElementId id, void* data)
{
  (void) ctx;
  (void) id;

  glhckRenderObject(*(glhckHandle*)data);
}


void guihckGlhckAddTextType(guihckContext* ctx)
{
  guihckElementTypeFunctionMap functionMap = {
    initText,
    destroyText,
    updateText,
    renderText,
    NULL,
    NULL
  };
  guihckElementTypeAdd(ctx, "text", functionMap, sizeof(_guihckGlhckText));
  guihckContextExecuteScript(ctx, GUIHCK_GLHCK_TEXT_SCM);
}

void initText(guihckContext* ctx, guihckElementId id, void* data)
{
  if(textThreadLocalContext.textRefs == 0)
  {
    textThreadLocalContext.text = glhckTextNew(1024, 1024);
    textThreadLocalContext.fonts = chckHashTableNew(16);
  }
  textThreadLocalContext.textRefs += 1;

  _guihckGlhckText* d = data;
  d->font = getFont("");
  d->object = glhckPlaneNew(1.0, 1.0);
  glhckHandle m = glhckMaterialNew(0);
  glhckObjectMaterial(d->object, m);
  glhckHandleRelease(m);
  d->content = NULL;
  d->fontPath = NULL;
  guihckElementAddParentPositionListeners(ctx, id);
  guihckElementAddUpdateProperty(ctx, id, "absolute-x");
  guihckElementAddUpdateProperty(ctx, id, "absolute-y");
  guihckElementAddUpdateProperty(ctx, id, "text");
  guihckElementAddUpdateProperty(ctx, id, "font");
  guihckElementAddUpdateProperty(ctx, id, "size");
  guihckElementAddUpdateProperty(ctx, id, "color");
}

void destroyText(guihckContext* ctx, guihckElementId id, void* data)
{
  (void) ctx;
  (void) id;

  _guihckGlhckText* d = data;
  glhckHandleRelease(d->object);
  if(d->content)
    free(d->content);

  textThreadLocalContext.textRefs -= 1;
  if(textThreadLocalContext.textRefs == 0)
  {
    chckHashTableIterator fontIter = {NULL, 0};
    glhckFont* font;
    while((font = chckHashTableIter(textThreadLocalContext.fonts, &fontIter)))
    {
      glhckTextFontFree(textThreadLocalContext.text, *font);
    }
    chckHashTableFree(textThreadLocalContext.fonts);
    textThreadLocalContext.fonts = NULL;

    glhckHandleRelease(textThreadLocalContext.text);
    textThreadLocalContext.text = 0;
  }
}

bool updateText(guihckContext* ctx, guihckElementId id, void* data)
{
  _guihckGlhckText* d = data;

  SCM textContent = guihckElementGetProperty(ctx, id, "text");
  SCM fontPath = guihckElementGetProperty(ctx, id, "font");
  SCM textSize = guihckElementGetProperty(ctx, id, "size");
  SCM c = guihckElementGetProperty(ctx, id, "color");

  if(scm_is_string(fontPath))
  {
    char* fontPathStr = scm_to_utf8_string(fontPath);
    if(!d->fontPath || strcmp(fontPathStr, d->fontPath))
    {
      d->font = getFont(fontPathStr);
      if(d->fontPath)
        free(d->fontPath);
      d->fontPath = fontPathStr;
    }
    else
    {
      free(fontPathStr);
    }
  }
  if(scm_is_string(textContent))
  {
    char* textContentStr = scm_to_utf8_string(textContent);
    if(!d->content || strcmp(textContentStr, d->content))
    {
      if(strlen(textContentStr) > 0)
      {
        float size = scm_is_real(textSize)  ? scm_to_double(textSize) : 12;
        glhckHandle texture = glhckTextRTT(textThreadLocalContext.text, d->font, size, textContentStr, glhckTextureDefaultLinearParameters());
        glhckMaterialTexture(glhckObjectGetMaterial(d->object), texture);
        if(texture)
          glhckHandleRelease(texture);
      }
      else
      {
        glhckMaterialTexture(glhckObjectGetMaterial(d->object), 0);
      }

      if(d->content)
        free(d->content);
      d->content = textContentStr;
    }
    else
    {
      free(textContentStr);
    }
  }

  float w = 0;
  float h = 0;
  glhckHandle texture = glhckMaterialGetTexture(glhckObjectGetMaterial(d->object));
  if(texture)
  {
    int textureWidth, textureHeight;
    glhckTextureGetInformation(texture, NULL, &textureWidth, &textureHeight, NULL, NULL, NULL, NULL);
    w = textureWidth;
    h = textureHeight;
    guihckElementProperty(ctx, id, "width", scm_from_double(w));
    guihckElementProperty(ctx, id, "height", scm_from_double(h));
  }

  SCM x = guihckElementGetProperty(ctx, id, "absolute-x");
  SCM y = guihckElementGetProperty(ctx, id, "absolute-y");
  kmVec3 position = *glhckObjectGetPosition(d->object);
  kmVec3 scale = *glhckObjectGetScale(d->object);

  scale.x = w/2;
  scale.y = h/2;
  if(scm_is_real(x)) position.x = scm_to_double(x) + scale.x;
  if(scm_is_real(y)) position.y = scm_to_double(y) + scale.y;

  glhckObjectPosition(d->object, &position);
  glhckObjectScale(d->object, &scale);

  if(scm_to_bool(scm_list_p(c)) && scm_to_int32(scm_length(c)) == 3)
  {
    glhckColor color = glhckMaterialGetDiffuse(glhckObjectGetMaterial(d->object));
    SCM r = scm_list_ref(c, scm_from_int8(0));
    SCM g = scm_list_ref(c, scm_from_int8(1));
    SCM b = scm_list_ref(c, scm_from_int8(2));

    unsigned char cr = (color & 0xFF000000) >> 24;
    unsigned char cg = (color & 0x00FF0000) >> 16;
    unsigned char cb = (color & 0x0000FF00) >> 8;
    unsigned char ca = (color & 0x000000FF);

    if(scm_to_bool(scm_integer_p(r))) cr = scm_to_uint8(r);
    if(scm_to_bool(scm_integer_p(g))) cg = scm_to_uint8(g);
    if(scm_to_bool(scm_integer_p(b))) cb = scm_to_uint8(b);

    glhckColor newColor = (ca | (cb << 8) | (cg << 16) | (cr << 24));
    glhckMaterialDiffuse(glhckObjectGetMaterial(d->object), newColor);
  }

  return false;
}

void renderText(guihckContext* ctx, guihckElementId id, void* data)
{
  (void) ctx;
  (void) id;

  _guihckGlhckText* d = data;
  glhckRenderObject(d->object);
}

unsigned int getFont(const char* fontPath)
{
  unsigned int* result = chckHashTableStrGet(textThreadLocalContext.fonts, fontPath);
  if(!result)
  {
    unsigned int font;

    if(strlen(fontPath) == 0)
    {
      font = glhckTextFontNewKakwafont(textThreadLocalContext.text, NULL);
    }
    else
    {
      font = glhckTextFontNew(textThreadLocalContext.text, fontPath);
    }
    chckHashTableStrSet(textThreadLocalContext.fonts, fontPath, &font, sizeof(unsigned int));
    return font;
  }

  return *result;
}

void guihckGlhckAddImageType(guihckContext* ctx)
{
  guihckElementTypeFunctionMap functionMap = {
    initImage,
    destroyImage,
    updateImage,
    renderImage,
    NULL,
    NULL
  };
  guihckElementTypeAdd(ctx, "image", functionMap, sizeof(_guihckGlhckImage));
  guihckContextExecuteScript(ctx, GUIHCK_GLHCK_IMAGE_SCM);
}

void initImage(guihckContext* ctx, guihckElementId id, void* data)
{
  _guihckGlhckImage* d = data;
  d->object = glhckPlaneNew(1.0, 1.0);
  glhckHandle m = glhckMaterialNew(0);
  glhckObjectMaterial(d->object, m);
  glhckHandleRelease(m);
  d->source = NULL;
  guihckElementAddParentPositionListeners(ctx, id);
  guihckElementAddUpdateProperty(ctx, id, "absolute-x");
  guihckElementAddUpdateProperty(ctx, id, "absolute-y");
  guihckElementAddUpdateProperty(ctx, id, "width");
  guihckElementAddUpdateProperty(ctx, id, "height");
  guihckElementAddUpdateProperty(ctx, id, "color");
  guihckElementAddUpdateProperty(ctx, id, "source");
}

void destroyImage(guihckContext* ctx, guihckElementId id, void* data)
{
  (void) ctx;
  (void) id;

  _guihckGlhckImage* d = data;
  glhckHandleRelease(d->object);
  if(d->source)
    free(d->source);
}

bool updateImage(guihckContext* ctx, guihckElementId id, void* data)
{
  _guihckGlhckImage* d = data;

  SCM source = guihckElementGetProperty(ctx, id, "source");

  if(scm_is_string(source))
  {
    char* sourceStr = scm_to_utf8_string(source);
    if(!d->source || strcmp(sourceStr, d->source))
    {
      glhckHandle texture = glhckTextureNewFromFile(sourceStr, NULL, glhckTextureDefaultSpriteParameters());
      glhckMaterialTexture(glhckObjectGetMaterial(d->object), texture);
      glhckHandleRelease(texture);
      free(d->source);
      d->source = sourceStr;
      int textureWidth, textureHeight;
      glhckTextureGetInformation(texture, NULL, &textureWidth, &textureHeight, NULL, NULL, NULL, NULL);
      guihckElementProperty(ctx, id, "source-width", scm_from_double(textureWidth));
      guihckElementProperty(ctx, id, "source-height", scm_from_double(textureHeight));

    }
    else
    {
      free(sourceStr);
    }
  }

  SCM width = guihckElementGetProperty(ctx, id, "width");
  SCM height = guihckElementGetProperty(ctx, id, "height");

  if(!scm_is_real(width))
  {
    width = guihckElementGetProperty(ctx, id, "source-width");
    guihckElementProperty(ctx, id, "width", width);
  }

  if(!scm_is_real(height))
  {
    height = guihckElementGetProperty(ctx, id, "source-height");
    guihckElementProperty(ctx, id, "height", height);
  }

  float w = scm_is_real(width) ? scm_to_double(width) : 0;
  float h = scm_is_real(height) ? scm_to_double(height) : 0;

  SCM x = guihckElementGetProperty(ctx, id, "absolute-x");
  SCM y = guihckElementGetProperty(ctx, id, "absolute-y");

  kmVec3 position = *glhckObjectGetPosition(d->object);
  kmVec3 scale = *glhckObjectGetScale(d->object);
  scale.x = w/2;
  scale.y = h/2;
  if(scm_is_real(x)) position.x = scm_to_double(x) + scale.x;
  if(scm_is_real(y)) position.y = scm_to_double(y) + scale.y;

  glhckObjectPosition(d->object, &position);
  glhckObjectScale(d->object, &scale);

  SCM c = guihckElementGetProperty(ctx, id, "color");
  if(scm_to_bool(scm_list_p(c)) && scm_to_int32(scm_length(c)) == 3)
  {
    glhckColor color = glhckMaterialGetDiffuse(glhckObjectGetMaterial(d->object));
    SCM r = scm_list_ref(c, scm_from_int8(0));
    SCM g = scm_list_ref(c, scm_from_int8(1));
    SCM b = scm_list_ref(c, scm_from_int8(2));

    unsigned char cr = (color & 0xFF000000) >> 24;
    unsigned char cg = (color & 0x00FF0000) >> 16;
    unsigned char cb = (color & 0x0000FF00) >> 8;
    unsigned char ca = (color & 0x000000FF);

    if(scm_to_bool(scm_integer_p(r))) cr = scm_to_uint8(r);
    if(scm_to_bool(scm_integer_p(g))) cg = scm_to_uint8(g);
    if(scm_to_bool(scm_integer_p(b))) cb = scm_to_uint8(b);

    glhckColor newColor = (ca | (cb << 8) | (cg << 16) | (cr << 24));
    glhckMaterialDiffuse(glhckObjectGetMaterial(d->object), newColor);
  }

  return false;
}

void renderImage(guihckContext* ctx, guihckElementId id, void* data)
{
  (void) ctx;
  (void) id;

  _guihckGlhckImage* d = data;
  glhckRenderObject(d->object);
}

void guihckGlhckAddTextInputType(guihckContext* ctx)
{
  guihckContextExecuteScript(ctx, GUIHCK_GLHCK_TEXT_INPUT_SCM);
}

