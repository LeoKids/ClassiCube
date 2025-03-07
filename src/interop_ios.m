#define GLES_SILENCE_DEPRECATION
#include "_WindowBase.h"
#include "Bitmap.h"
#include "Input.h"
#include "Platform.h"
#include "String.h"
#include "Errors.h"
#include "Drawer2D.h"
#include "Launcher.h"
#include "LBackend.h"
#include "LWidgets.h"
#include "LScreens.h"
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <UIKit/UIPasteboard.h>
#include <UIKit/UIKit.h>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

@interface CCWindow : UIWindow
@end

@interface CCViewController : UIViewController
@end

@interface CCAppDelegate : UIResponder<UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

static CCViewController* controller;
static UIWindow* win_handle;
static UIView* view_handle;

static void AddTouch(UITouch* t) {
    CGPoint loc = [t locationInView:view_handle];
    int x = loc.x, y = loc.y; long ui_id = (long)t;
    Platform_Log3("POINTER %x - DOWN %i,%i", &ui_id, &x, &y);
    Input_AddTouch((long)t, loc.x, loc.y);
}

static void UpdateTouch(UITouch* t) {
    CGPoint loc = [t locationInView:view_handle];
    int x = loc.x, y = loc.y; long ui_id = (long)t;
    Platform_Log3("POINTER %x - MOVE %i,%i", &ui_id, &x, &y);
    Input_UpdateTouch((long)t, loc.x, loc.y);
}

static void RemoveTouch(UITouch* t) {
    CGPoint loc = [t locationInView:view_handle];
    int x = loc.x, y = loc.y; long ui_id = (long)t;
    Platform_Log3("POINTER %x - UP %i,%i", &ui_id, &x, &y);
    Input_RemoveTouch((long)t, loc.x, loc.y);
}

@implementation CCWindow

//- (void)drawRect:(CGRect)dirty { DoDrawFramebuffer(dirty); }

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch* t in touches) AddTouch(t);
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch* t in touches) UpdateTouch(t);
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch* t in touches) RemoveTouch(t);
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch* t in touches) RemoveTouch(t);
}

- (BOOL)isOpaque { return YES; }

// helpers for LBackend
static void LBackend_HandleButton(id btn);
static void LBackend_HandleInput(id ipt);

- (void)handleButtonPress:(id)sender {
    LBackend_HandleButton(sender);
}

- (void)handleTextChanged:(id)sender {
    LBackend_HandleInput(sender);
}

@end

static cc_bool landscape_locked;
@implementation CCViewController
- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    if (landscape_locked)
        return UIInterfaceOrientationMaskLandscape;
    return [super supportedInterfaceOrientations];
}
@end

@implementation CCAppDelegate

- (void)runMainLoop {
    extern int main_real(int argc, char** argv);
    main_real(1, NULL);
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Override point for customization after application launch.
    // schedule the actual main loop to run in next CFRunLoop iteration
    //  (as calling main_real here doesn't work properly)
    [self performSelector:@selector(runMainLoop) withObject:nil afterDelay:0.0];
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
    Platform_LogConst("INACTIVE");
    WindowInfo.Focused = false;
    Event_RaiseVoid(&WindowEvents.FocusChanged);
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    Platform_LogConst("BACKGROUND");
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
    Platform_LogConst("FOREGROUND");
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
    Platform_LogConst("ACTIVE");
    WindowInfo.Focused = true;
    Event_RaiseVoid(&WindowEvents.FocusChanged);
}

- (void)applicationWillTerminate:(UIApplication *)application {
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
    // TODO implement somehow, prob need a variable in Program.c
}
@end

int main(int argc, char * argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([CCAppDelegate class]));
    }
}

void Clipboard_GetText(cc_string* value) {
	const char* raw;
	NSString* str;

	str = [UIPasteboard generalPasteboard].string;
	if (!str) return;

	raw = str.UTF8String;
	String_AppendUtf8(value, raw, String_Length(raw));
}

void Clipboard_SetText(const cc_string* value) {
	char raw[NATIVE_STR_LEN];
	NSString* str;

	Platform_EncodeUtf8(raw, value);
	str = [NSString stringWithUTF8String:raw];
	[UIPasteboard generalPasteboard].string = str;
}

/*########################################################################################################################*
*---------------------------------------------------------Window----------------------------------------------------------*
*#########################################################################################################################*/
void Cursor_GetRawPos(int* x, int* y) { *x = 0; *y = 0; }
void Cursor_SetPosition(int x, int y) { }
void Cursor_DoSetVisible(cc_bool visible) { }

void Window_SetTitle(const cc_string* title) {
	// TODO: Implement this somehow
}

void Window_Init(void) {
    WindowInfo.SoftKeyboard = SOFT_KEYBOARD_RESIZE;
    Input_SetTouchMode(true);
    
    DisplayInfo.Depth  = 32;
    DisplayInfo.ScaleX = 1; // TODO dpi scale
    DisplayInfo.ScaleY = 1; // TODO dpi scale
}

static CGRect DoCreateWindow(void) {
    CGRect bounds = UIScreen.mainScreen.bounds;
    controller = [CCViewController alloc];
    win_handle = [[CCWindow alloc] initWithFrame:bounds];
    
    win_handle.rootViewController = controller;
    win_handle.backgroundColor = UIColor.blueColor;
    WindowInfo.Exists = true;
    WindowInfo.Width  = bounds.size.width;
    WindowInfo.Height = bounds.size.height;
    return bounds;
}
void Window_SetSize(int width, int height) { }

void Window_Show(void) {
    [win_handle makeKeyAndVisible];
}

void Window_Close(void) {
    WindowInfo.Exists = false;
    Event_RaiseVoid(&WindowEvents.Closing);
}

void Window_ProcessEvents(void) {
    SInt32 res;
    // manually tick event queue
    do {
        res = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, TRUE);
    } while (res == kCFRunLoopRunHandledSource);
}
void ShowDialogCore(const char* title, const char* msg) {
    Platform_LogConst(title);
    Platform_LogConst(msg);
    NSString* _title = [NSString stringWithCString:title encoding:NSASCIIStringEncoding];
    NSString* _msg   = [NSString stringWithCString:msg encoding:NSASCIIStringEncoding];
    __block int completed = false;
    
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:_title message:_msg preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction* okBtn     = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction* act) { completed = true; }];
    [alert addAction:okBtn];
    [controller presentViewController:alert animated:YES completion: Nil];
    
    // TODO clicking outside message box crashes launcher
    // loop until alert is closed TODO avoid sleeping
    while (!completed) {
        Window_ProcessEvents();
        Thread_Sleep(16);
    }
}

static UITextField* text_input;
void Window_OpenKeyboard(const struct OpenKeyboardArgs* args) {
    text_input = [[UITextField alloc] initWithFrame:CGRectZero];
    text_input.hidden = YES;
    [view_handle addSubview:text_input];
    [text_input becomeFirstResponder];
}

void Window_SetKeyboardText(const cc_string* text) {
    char raw[NATIVE_STR_LEN];
    NSString* str;
    
    Platform_EncodeUtf8(raw, text);
    str = [NSString stringWithUTF8String:raw];
    text_input.text = str;
}

void Window_CloseKeyboard(void) {
    [text_input resignFirstResponder];
}

int Window_GetWindowState(void) { return WINDOW_STATE_NORMAL; }
cc_result Window_EnterFullscreen(void) { return ERR_NOT_SUPPORTED; }
cc_result Window_ExitFullscreen(void) { return ERR_NOT_SUPPORTED; }
int Window_IsObscured(void) { return 0; }

void Window_EnableRawMouse(void)  { }
void Window_UpdateRawMouse(void)  { }
void Window_DisableRawMouse(void) { }

void Window_LockLandscapeOrientation(cc_bool lock) {
    // TODO doesn't work
    landscape_locked = lock;
    [UIViewController attemptRotationToDeviceOrientation];
}

cc_result Window_OpenFileDialog(const char* const* filters, OpenFileDialogCallback callback) {
	return ERR_NOT_SUPPORTED;
}


/*#########################################################################################################################*
 *--------------------------------------------------------2D window--------------------------------------------------------*
 *#########################################################################################################################*/
void Window_Create2D(int width, int height) {
    CGRect bounds = DoCreateWindow();
    
    view_handle = [[UIView alloc] initWithFrame:bounds];
    view_handle.multipleTouchEnabled = true;
    controller.view = view_handle;
}

static CGContextRef win_ctx;
static struct Bitmap fb_bmp;
void Window_AllocFramebuffer(struct Bitmap* bmp) {
    bmp->scan0 = (BitmapCol*)Mem_Alloc(bmp->width * bmp->height, 4, "window pixels");
    fb_bmp = *bmp;
    
    win_ctx = CGBitmapContextCreate(bmp->scan0, bmp->width, bmp->height, 8, bmp->width * 4,
                                    CGColorSpaceCreateDeviceRGB(), kCGBitmapByteOrder32Host | kCGImageAlphaNoneSkipFirst);
}

void Window_DrawFramebuffer(Rect2D r) {
    CGRect rect;
    rect.origin.x    = r.X;
    rect.origin.y    = WindowInfo.Height - r.Y - r.Height;
    rect.size.width  = r.Width;
    rect.size.height = r.Height;
    win_handle.layer.contents = CFBridgingRelease(CGBitmapContextCreateImage(win_ctx));
    // TODO always redraws entire launcher which is quite terrible performance wise
    //[win_handle setNeedsDisplayInRect:rect];
}

void Window_FreeFramebuffer(struct Bitmap* bmp) {
    Mem_Free(bmp->scan0);
    CGContextRelease(win_ctx);
}


/*#########################################################################################################################*
 *--------------------------------------------------------3D window--------------------------------------------------------*
 *#########################################################################################################################*/
@interface CCGLView : UIView
@end

@implementation CCGLView

+ (Class)layerClass {
    return [CAEAGLLayer class];
}
@end

void Window_Create3D(int width, int height) {
    CGRect bounds = DoCreateWindow();
    view_handle = [[CCGLView alloc] initWithFrame:bounds];
    view_handle.multipleTouchEnabled = true;
    controller.view = view_handle;
    
    CAEAGLLayer* layer = (CAEAGLLayer*)view_handle.layer;
    layer.opaque = YES;
    layer.drawableProperties =
   @{
        kEAGLDrawablePropertyRetainedBacking : [NSNumber numberWithBool:NO],
        kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8
    };
}


/*########################################################################################################################*
*--------------------------------------------------------GLContext--------------------------------------------------------*
*#########################################################################################################################*/
static EAGLContext* ctx_handle;
static GLuint framebuffer;
static GLuint color_renderbuffer, depth_renderbuffer;

static void CreateFramebuffer(void) {
    CAEAGLLayer* layer = (CAEAGLLayer*)view_handle.layer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    
    glGenRenderbuffers(1, &color_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, color_renderbuffer);
    [ctx_handle renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer];
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_renderbuffer);
    
    glGenRenderbuffers(1, &depth_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, WindowInfo.Width, WindowInfo.Height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer);
    
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        Logger_Abort2(status, "Failed to create renderbuffer");
}

void GLContext_Create(void) {
    ctx_handle = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    [EAGLContext setCurrentContext:ctx_handle];
    
    // unlike other platforms, have to manually setup render framebuffer
    CreateFramebuffer();
}
                  
void GLContext_Update(void) { }
cc_bool GLContext_TryRestore(void) { return false; }

void GLContext_Free(void) {
    [EAGLContext setCurrentContext:Nil];
}

void* GLContext_GetAddress(const char* function) { return NULL; }

cc_bool GLContext_SwapBuffers(void) {
    static GLenum discards[] = { GL_DEPTH_ATTACHMENT };
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glDiscardFramebufferEXT(GL_FRAMEBUFFER, 1, discards);
    glBindRenderbuffer(GL_RENDERBUFFER, color_renderbuffer);
    [ctx_handle presentRenderbuffer:GL_RENDERBUFFER];
    return true;
}
void GLContext_SetFpsLimit(cc_bool vsync, float minFrameMs) { }
void GLContext_GetApiInfo(cc_string* info) { }
const struct UpdaterInfo Updater_Info = { "&eCompile latest source code to update", 0 };


/*########################################################################################################################*
 *--------------------------------------------------------Updater----------------------------------------------------------*
 *#########################################################################################################################*/
const char* const Updater_OGL  = NULL;
const char* const Updater_D3D9 = NULL;
cc_bool Updater_Clean(void) { return true; }

cc_result Updater_GetBuildTime(cc_uint64* t) {
    char path[NATIVE_STR_LEN + 1] = { 0 };
    uint32_t size = NATIVE_STR_LEN;
    if (_NSGetExecutablePath(path, &size)) return ERR_INVALID_ARGUMENT;
    
    struct stat sb;
    if (stat(path, &sb) == -1) return errno;
    *t = (cc_uint64)sb.st_mtime;
    return 0;
}

cc_result Updater_Start(const char** action)   { *action = "Updating game"; return ERR_NOT_SUPPORTED; }
cc_result Updater_MarkExecutable(void)         { return 0; }
cc_result Updater_SetNewBuildTime(cc_uint64 t) { return ERR_NOT_SUPPORTED; }


/*########################################################################################################################*
 *--------------------------------------------------------Platform--------------------------------------------------------*
 *#########################################################################################################################*/
static char gameArgs[GAME_MAX_CMDARGS][STRING_SIZE];
static int gameNumArgs;

cc_result Process_StartOpen(const cc_string* args) {
    char raw[NATIVE_STR_LEN];
    NSURL* url;
    NSString* str;
    
    Platform_EncodeUtf8(raw, args);
    str = [NSString stringWithUTF8String:raw];
    url = [[NSURL alloc] initWithString:str];
    [UIApplication.sharedApplication openURL:url];
    return 0;
}

cc_result Process_StartGame2(const cc_string* args, int numArgs) {
    for (int i = 0; i < numArgs; i++) {
        String_CopyToRawArray(gameArgs[i], &args[i]);
    }

    gameNumArgs = numArgs;
    return 0;
}

int Platform_GetCommandLineArgs(int argc, STRING_REF char** argv, cc_string* args) {
    int count = gameNumArgs;
    for (int i = 0; i < count; i++) {
        args[i] = String_FromRawArray(gameArgs[i]);
    }

    // clear arguments so after game is closed, launcher is started
    gameNumArgs = 0;
    return count;
}

cc_result Platform_SetDefaultCurrentDirectory(int argc, char **argv) {
    // TODO this is the API should actually be using.. eventually
    /*NSArray* array = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    if ([array count] <= 0) return ERR_NOT_SUPPORTED;
    
    NSString* str = [array objectAtIndex:0];
    const char* name = [str fileSystemRepresentation];
    return chdir(name) == -1 ? errno : 0;*/
    
    char path[NATIVE_STR_LEN + 1] = { 0 };
    uint32_t size = NATIVE_STR_LEN;
    if (_NSGetExecutablePath(path, &size)) return ERR_INVALID_ARGUMENT;
    
    // despite what you'd assume, size is NOT changed to length of path
    int len = String_CalcLen(path, NATIVE_STR_LEN);
    
    // get rid of filename at end of directory
    for (int i = len - 1; i >= 0; i--, len--) {
        if (path[i] == '/') break;
    }

    path[len] = '\0';
    return chdir(path) == -1 ? errno : 0;
}

void Platform_ShareScreenshot(const cc_string* filename) {
    cc_string path; char pathBuffer[FILENAME_SIZE];
    String_InitArray(path, pathBuffer);
    char tmp[NATIVE_STR_LEN];
    
    String_Format1(&path, "screenshots/%s", filename);
    Platform_EncodeUtf8(tmp, &path);
    
    // TODO unify with ToNSString
    NSString* pathStr = [NSString stringWithUTF8String:tmp];
    UIImage* img = [UIImage imageWithContentsOfFile:pathStr];
    
    // https://stackoverflow.com/questions/31955140/sharing-image-using-uiactivityviewcontroller
    UIActivityViewController* act;
    act = [UIActivityViewController alloc];
    act = [act initWithActivityItems:@[ @"Share screenshot via", img] applicationActivities:Nil];
    [controller presentViewController:act animated:true completion:Nil];
}

void GetDeviceUUID(cc_string* str) {
    UIDevice* device = [UIDevice currentDevice];
    NSString* string = [[device identifierForVendor] UUIDString];
    
    // TODO avoid code duplication
    const char* src = [string UTF8String];
    String_AppendUtf8(str, src, String_Length(src));
}


/*########################################################################################################################*
 *------------------------------------------------------UI Backend--------------------------------------------------------*
 *#########################################################################################################################*/
static UIColor* ToUIColor(BitmapCol color, float A) {
    float R = BitmapCol_R(color) / 255.0f;
    float G = BitmapCol_G(color) / 255.0f;
    float B = BitmapCol_B(color) / 255.0f;
    return [UIColor colorWithRed:R green:G blue:B alpha:A];
}

static NSString* ToNSString(const cc_string* text) {
    char raw[NATIVE_STR_LEN];
    Platform_EncodeUtf8(raw, text);
    return [NSString stringWithUTF8String:raw];
}

static void FreeContents(void* info, const void* data, size_t size) { Mem_Free(data); }
// TODO probably a better way..
static UIImage* ToUIImage(struct Bitmap* bmp) {
    CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
    CGDataProviderRef provider;
    CGImageRef image;

    provider = CGDataProviderCreateWithData(NULL, bmp->scan0,
                                            Bitmap_DataSize(bmp->width, bmp->height), FreeContents);
    image    = CGImageCreate(bmp->width, bmp->height, 8, 32, bmp->width * 4, colorspace,
                             kCGBitmapByteOrder32Host | kCGImageAlphaNoneSkipFirst, provider, NULL, 0, 0);
    
    UIImage* img = [UIImage imageWithCGImage:image];
    
    CGImageRelease(image);
    CGDataProviderRelease(provider);
    CGColorSpaceRelease(colorspace);
    return img;
}

void LBackend_Init(void) {
}

static void UpdateWidgetDimensions(void* widget) {
    struct LWidget* w = widget;
    UIView* view = (__bridge UIView*)w->meta;
    
    CGRect rect = [view frame];
    w->width    = (int)rect.size.width;
    w->height   = (int)rect.size.height;
}

void LBackend_WidgetRepositioned(struct LWidget* w) {
    UIView* view   = (__bridge UIView*)w->meta;
    
    CGRect rect = [view frame];
    rect.origin.x = w->x;
    rect.origin.y = w->y;
    [view setFrame:rect];
}

struct LScreen* active;
void LBackend_SetScreen(struct LScreen* s) {
    active = s;
    
    for (int i = 0; i < s->numWidgets; i++) {
        void* obj = s->widgets[i]->meta;
        if (!obj) continue;
        
        UIView* view = (__bridge UIView*)obj;
        [view_handle addSubview:view];
    }
}

void LBackend_CloseScreen(struct LScreen* s) {
    if (!s) return;
    
    // remove all widgets from previous screen
    NSArray<UIView*>* elems = [view_handle subviews];
    for (UIView* view in elems) {
        [view removeFromSuperview];
    }
}

static void AssignView(void* widget, UIView* view) {
    struct LWidget* w = widget;
    // doesn't work, the view get auto garbage collected
    //  after LBackend_CloseScreen removes the subviews
    //w->meta = (__bridge void*)view;
    w->meta = CFBridgingRetain(view);
}

static struct LWidget* FindWidgetForView(id obj) {
    for (int i = 0; i < active->numWidgets; i++) {
        void* meta = active->widgets[i]->meta;
        if (meta != (__bridge void*)obj) continue;
        
        return active->widgets[i];
    }
    return NULL;
}

/*########################################################################################################################*
 *------------------------------------------------------ButtonWidget-------------------------------------------------------*
 *#########################################################################################################################*/
static void LBackend_HandleButton(id btn_obj) {
    struct LWidget* w = FindWidgetForView(btn_obj);
    if (w == NULL) return;
    
    struct LButton* btn = (struct LButton*)w;
    btn->OnClick(btn);
}

void LBackend_ButtonInit(struct LButton* w, int width, int height) {
    UIButton* btn = [[UIButton alloc] init];
    btn.frame = CGRectMake(0, 0, width, height);
    // TODO should be app_handle, because win_handle can change
    [btn addTarget:win_handle action:@selector(handleButtonPress:) forControlEvents:UIControlEventTouchUpInside];
    
    AssignView(w, btn);
    UpdateWidgetDimensions(w);
    // memory freeing deferred until UIImage is freed (see FreeContents)
    struct Bitmap bmp1, bmp2;
    
    Bitmap_Allocate(&bmp1, w->width, w->height);
    LButton_DrawBackground(w, &bmp1, 0, 0);
    [btn setBackgroundImage:ToUIImage(&bmp1) forState:UIControlStateNormal];
    
    Bitmap_Allocate(&bmp2, w->width, w->height);
    w->hovered = true;
    LButton_DrawBackground(w, &bmp2, 0, 0);
    [btn setBackgroundImage:ToUIImage(&bmp2) forState:UIControlStateHighlighted];
}

void LBackend_ButtonUpdate(struct LButton* w) {
    UIButton* btn = (__bridge UIButton*)w->meta;
    NSString* str = ToNSString(&w->text);
    
    [btn setTitle:str forState:UIControlStateNormal];
    UpdateWidgetDimensions(w);
}


void LBackend_ButtonDraw(struct LButton* w) {
}


/*########################################################################################################################*
 *-----------------------------------------------------CheckboxWidget------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_CheckboxInit(struct LCheckbox* w) {
    UISwitch* swt = [[UISwitch alloc] init];
    
    AssignView(w, swt);
    UpdateWidgetDimensions(w);
}

void LBackend_CheckboxDraw(struct LCheckbox* w) {
}


/*########################################################################################################################*
 *------------------------------------------------------InputWidget--------------------------------------------------------*
 *#########################################################################################################################*/
static void LBackend_HandleInput(id ipt_obj) {
    struct LWidget* w = FindWidgetForView(ipt_obj);
    if (w == NULL) return;
    
    UITextField* src = (UITextField*)ipt_obj;
    const char* str  = [[src text] UTF8String];
    
    struct LInput* ipt = (struct LInput*)w;
    ipt->text.length   = 0;
    String_AppendUtf8(&ipt->text, str, String_Length(str));
    if (ipt->TextChanged) ipt->TextChanged(ipt);
}

void LBackend_InputInit(struct LInput* w, int width) {
    UITextField* fld = [[UITextField alloc] init];
    fld.frame           = CGRectMake(0, 0, width, 30);
    fld.borderStyle     = UITextBorderStyleBezel;
    fld.backgroundColor = [UIColor whiteColor];
    // TODO should be app_handle, because win_handle can change
    [fld addTarget:win_handle action:@selector(handleTextChanged:) forControlEvents:UIControlEventEditingChanged];
    
    if (w->type == KEYBOARD_TYPE_INTEGER) {
        [fld setKeyboardType:UIKeyboardTypeNumberPad];
    } else if (w->type == KEYBOARD_TYPE_PASSWORD) {
        fld.secureTextEntry = YES;
    }
    
    if (w->hintText) {
        cc_string hint  = String_FromReadonly(w->hintText);
        fld.placeholder = ToNSString(&hint);
    }
    
    AssignView(w, fld);
    UpdateWidgetDimensions(w);
}

void LBackend_InputUpdate(struct LInput* w) {
    UITextField* fld = (__bridge UITextField*)w->meta;
    fld.text         = ToNSString(&w->text);
    UpdateWidgetDimensions(w);
}

void LBackend_InputDraw(struct LInput* w) {
}

void LBackend_InputTick(struct LInput* w) { }
void LBackend_InputSelect(struct LInput* w, int idx, cc_bool wasSelected) { }
void LBackend_InputUnselect(struct LInput* w) { }


/*########################################################################################################################*
 *------------------------------------------------------LabelWidget--------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_LabelInit(struct LLabel* w) {
    UILabel* lbl  = [[UILabel alloc] init];
    lbl.textColor = [UIColor whiteColor];
    
    AssignView(w, lbl);
    UpdateWidgetDimensions(w);
}

void LBackend_LabelUpdate(struct LLabel* w) {
    UILabel* lbl = (__bridge UILabel*)w->meta;
    char raw[NATIVE_STR_LEN];
    Platform_EncodeUtf8(raw, &w->text);
    
    NSString* str = [NSString stringWithUTF8String:raw];
    lbl.text = str;
    [lbl sizeToFit]; // adjust label to fit text
    
    UpdateWidgetDimensions(w);
}

void LBackend_LabelDraw(struct LLabel* w) {
}


/*########################################################################################################################*
 *-------------------------------------------------------LineWidget--------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_LineInit(struct LLine* w, int width) {
    UIView* view = [[UIView alloc] init];
    view.frame   = CGRectMake(0, 0, width, 2);
    
    BitmapCol color = LLine_GetColor();
    view.backgroundColor = ToUIColor(color, 0.5f);
    
    AssignView(w, view);
    UpdateWidgetDimensions(w);
}

void LBackend_LineDraw(struct LLine* w) {
}


/*########################################################################################################################*
 *------------------------------------------------------SliderWidget-------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_SliderInit(struct LSlider* w, int width, int height) {
    UIProgressView* prg = [[UIProgressView alloc] init];
    prg.frame = CGRectMake(0, 0, width, height);
    
    AssignView(w, prg);
    UpdateWidgetDimensions(w);
}

void LBackend_SliderUpdate(struct LSlider* w) {
    UIProgressView* lbl = (__bridge UIProgressView*)w->meta;
    
    lbl.progress = w->value / 100.0f;
}

void LBackend_SliderDraw(struct LSlider* w) {
}


/*########################################################################################################################*
 *------------------------------------------------------TableWidget-------------------------------------------------------*
 *#########################################################################################################################*/
void LBackend_TableInit(struct LTableView* w) {
    UITableView* tbl = [[UITableView alloc] init];
    tbl.frame = CGRectMake(0, 0, 200, 30);
    
    AssignView(w, tbl);
    UpdateWidgetDimensions(w);
}

void LBackend_TableDraw(struct LTable* w) { }
void LBackend_TableReposition(struct LTable* w) { }

void LBackend_TableMouseDown(struct LTable* w, int idx) { }
void LBackend_TableMouseUp(struct   LTable* w, int idx) { }
void LBackend_TableMouseMove(struct LTable* w, int idx) { }
