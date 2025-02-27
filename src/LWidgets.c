#include "LWidgets.h"
#ifndef CC_BUILD_WEB
#include "String.h"
#include "Gui.h"
#include "Drawer2D.h"
#include "Launcher.h"
#include "ExtMath.h"
#include "Window.h"
#include "Funcs.h"
#include "LWeb.h"
#include "Platform.h"
#include "LScreens.h"
#include "Input.h"
#include "Utils.h"
#include "LBackend.h"

static int xInputOffset;
static int flagXOffset, flagYOffset;
static int xBorder, xBorder2, xBorder4;
static int yBorder, yBorder2, yBorder4;

void LWidget_CalcOffsets(void) {
	xBorder = Display_ScaleX(1); xBorder2 = xBorder * 2; xBorder4 = xBorder * 4;
	yBorder = Display_ScaleY(1); yBorder2 = yBorder * 2; yBorder4 = yBorder * 4;

	xInputOffset = Display_ScaleX(5);
	flagXOffset  = Display_ScaleX(2);
	flagYOffset  = Display_ScaleY(6);
}

void LWidget_SetLocation(void* widget, cc_uint8 horAnchor, cc_uint8 verAnchor, int xOffset, int yOffset) {
	struct LWidget* w = (struct LWidget*)widget;
	w->horAnchor = horAnchor; w->verAnchor = verAnchor;
	w->xOffset   = xOffset;   w->yOffset   = yOffset;
	LWidget_CalcPosition(widget);
}

void LWidget_CalcPosition(void* widget) {
	struct LWidget* w = (struct LWidget*)widget;
	w->x = Gui_CalcPos(w->horAnchor, Display_ScaleX(w->xOffset), w->width,  WindowInfo.Width);
	w->y = Gui_CalcPos(w->verAnchor, Display_ScaleY(w->yOffset), w->height, WindowInfo.Height);
	LBackend_WidgetRepositioned(w);
}

void LWidget_Draw(void* widget) {
	struct LWidget* w = (struct LWidget*)widget;
	w->last.X = w->x; w->last.Width  = w->width;
	w->last.Y = w->y; w->last.Height = w->height;

	w->VTABLE->Draw(w);
	Launcher_MarkDirty(w->x, w->y, w->width, w->height);
}

void LWidget_Redraw(void* widget) {
	struct LWidget* w = (struct LWidget*)widget;
	Launcher_ResetArea(w->last.X, w->last.Y, w->last.Width, w->last.Height);
	LWidget_Draw(w);
}


/*########################################################################################################################*
*------------------------------------------------------ButtonWidget-------------------------------------------------------*
*#########################################################################################################################*/
static BitmapCol LButton_Expand(BitmapCol a, int amount) {
	int r, g, b;
	r = BitmapCol_R(a) + amount; Math_Clamp(r, 0, 255);
	g = BitmapCol_G(a) + amount; Math_Clamp(g, 0, 255);
	b = BitmapCol_B(a) + amount; Math_Clamp(b, 0, 255);
	return BitmapCol_Make(r, g, b, 255);
}

static void LButton_DrawBase(struct LButton* w, struct Bitmap* bmp, int x, int y) {
	BitmapCol color = w->hovered ? Launcher_Theme.ButtonForeActiveColor 
								 : Launcher_Theme.ButtonForeColor;

	if (Launcher_Theme.ClassicBackground) {
		Gradient_Noise(bmp, color, 8,
						x + xBorder,           y + yBorder,
						w->width - xBorder2,   w->height - yBorder2);
	} else {
		
		Gradient_Vertical(bmp, LButton_Expand(color, 8), LButton_Expand(color, -8),
						  x + xBorder,         y + yBorder,
						  w->width - xBorder2, w->height - yBorder2);
	}
}

static void LButton_DrawBorder(struct LButton* w, struct Bitmap* bmp, int x, int y) {
	BitmapCol backColor = Launcher_Theme.ButtonBorderColor;
	Drawer2D_Clear(bmp, backColor, 
					x + xBorder,            y,
					w->width - xBorder2,    yBorder);
	Drawer2D_Clear(bmp, backColor,
					x + xBorder,            y + w->height - yBorder,
					w->width - xBorder2,    yBorder);
	Drawer2D_Clear(bmp, backColor,
					x,                      y + yBorder,
					xBorder,                w->height - yBorder2);
	Drawer2D_Clear(bmp, backColor,
					x + w->width - xBorder, y + yBorder,
					xBorder,                w->height - yBorder2);
}

static void LButton_DrawHighlight(struct LButton* w, struct Bitmap* bmp, int x, int y) {
	BitmapCol activeColor = BitmapCol_Make(189, 198, 255, 255);
	BitmapCol color       = Launcher_Theme.ButtonHighlightColor;

	if (Launcher_Theme.ClassicBackground) {
		if (w->hovered) color = activeColor;

		Drawer2D_Clear(&Launcher_Framebuffer, color,
						x + xBorder2,         y + yBorder,
						w->width - xBorder4,  yBorder);
		Drawer2D_Clear(&Launcher_Framebuffer, color,
						x + xBorder,          y + yBorder2,
						xBorder,              w->height - yBorder4);
	} else if (!w->hovered) {
		Drawer2D_Clear(&Launcher_Framebuffer, color,
						x + xBorder2,         y + yBorder,
						w->width - xBorder4,  yBorder);
	}
}

void LButton_DrawBackground(struct LButton* w, struct Bitmap* bmp, int x, int y) {
	LButton_DrawBase(w,      bmp, x, y);
	LButton_DrawBorder(w,    bmp, x, y);
	LButton_DrawHighlight(w, bmp, x, y);
}

static void LButton_Draw(void* widget) {
	struct LButton* w = (struct LButton*)widget;
	LBackend_ButtonDraw(w);
	Launcher_MarkDirty(w->x, w->y, w->width, w->height);
}

static void LButton_Hover(void* w, int idx, cc_bool wasOver) {
	/* only need to redraw when changing from unhovered to hovered */
	if (!wasOver) LWidget_Draw(w); 
}

static const struct LWidgetVTABLE lbutton_VTABLE = {
	LButton_Draw, NULL,
	NULL, NULL,                  /* Key    */
	LButton_Hover, LWidget_Draw, /* Hover  */
	NULL, NULL                   /* Select */
};
void LButton_Init(struct LButton* w, int width, int height, const char* text) {
	w->VTABLE = &lbutton_VTABLE;
	w->tabSelectable = true;
	LBackend_ButtonInit(w, width, height);
	LButton_SetConst(w, text);
}

void LButton_SetConst(struct LButton* w, const char* text) {
	w->text = String_FromReadonly(text);
	LBackend_ButtonUpdate(w);
}


/*########################################################################################################################*
*-----------------------------------------------------CheckboxWidget------------------------------------------------------*
*#########################################################################################################################*/
static void LCheckbox_Draw(void* widget) {
	struct LCheckbox* w = (struct LCheckbox*)widget;
	LBackend_CheckboxDraw(w);
	Launcher_MarkDirty(w->x, w->y, w->width, w->height);
}

static const struct LWidgetVTABLE lcheckbox_VTABLE = {
	LCheckbox_Draw, NULL,
	NULL, NULL, /* Key    */
	NULL, NULL, /* Hover  */
	NULL, NULL  /* Select */
};
void LCheckbox_Init(struct LCheckbox* w, const char* text) {
	w->VTABLE = &lcheckbox_VTABLE;
	w->tabSelectable = true;

	String_InitArray(w->text, w->_textBuffer);
	String_AppendConst(&w->text, text);
	LBackend_CheckboxInit(w);
}


/*########################################################################################################################*
*------------------------------------------------------InputWidget--------------------------------------------------------*
*#########################################################################################################################*/
void LInput_UNSAFE_GetText(struct LInput* w, cc_string* text) {
	int i;
	if (w->type != KEYBOARD_TYPE_PASSWORD) { *text = w->text; return; }

	for (i = 0; i < w->text.length; i++) {
		String_Append(text, '*');
	}
}

static void LInput_Draw(void* widget) {
	struct LInput* w = (struct LInput*)widget;
	LBackend_InputDraw(w);
}

static void LInput_TickCaret(void* widget) {
	struct LInput* w = (struct LInput*)widget;
	LBackend_InputTick(w);
}

static void LInput_Select(void* widget, int idx, cc_bool wasSelected) {
	struct LInput* w = (struct LInput*)widget;
	LBackend_InputSelect(w, idx, wasSelected);
}

static void LInput_Unselect(void* widget, int idx) {
	struct LInput* w = (struct LInput*)widget;
	LBackend_InputUnselect(w);
}

static void LInput_AdvanceCaretPos(struct LInput* w, cc_bool forwards) {
	if (forwards && w->caretPos == -1) return;
	if (!forwards && w->caretPos == 0) return;
	if (w->caretPos == -1 && !forwards) /* caret after text */
		w->caretPos = w->text.length;

	w->caretPos += (forwards ? 1 : -1);
	if (w->caretPos < 0 || w->caretPos >= w->text.length) w->caretPos = -1;
	LWidget_Redraw(w);
}

static void LInput_CopyFromClipboard(struct LInput* w) {
	cc_string text; char textBuffer[2048];
	String_InitArray(text, textBuffer);

	Clipboard_GetText(&text);
	String_UNSAFE_TrimStart(&text);
	String_UNSAFE_TrimEnd(&text);

	if (w->ClipboardFilter) w->ClipboardFilter(&text);
	LInput_AppendString(w, &text);
}

/* If caret position is now beyond end of text, resets to -1 */
static CC_INLINE void LInput_ClampCaret(struct LInput* w) {
	if (w->caretPos >= w->text.length) w->caretPos = -1;
}

/* Removes the character preceding the caret in the currently entered text */
static void LInput_Backspace(struct LInput* w) {
	if (!w->text.length || w->caretPos == 0) return;

	if (w->caretPos == -1) {
		String_DeleteAt(&w->text, w->text.length - 1);
	} else {	
		String_DeleteAt(&w->text, w->caretPos - 1);
		w->caretPos--;
		if (w->caretPos == -1) w->caretPos = 0;
	}

	if (w->TextChanged) w->TextChanged(w);
	LInput_ClampCaret(w);
	LWidget_Redraw(w);
}

/* Removes the character at the caret in the currently entered text */
static void LInput_Delete(struct LInput* w) {
	if (!w->text.length || w->caretPos == -1) return;

	String_DeleteAt(&w->text, w->caretPos);
	if (w->caretPos == -1) w->caretPos = 0;

	if (w->TextChanged) w->TextChanged(w);
	LInput_ClampCaret(w);
	LWidget_Redraw(w);
}

static void LInput_KeyDown(void* widget, int key, cc_bool was) {
	struct LInput* w = (struct LInput*)widget;
	if (key == KEY_BACKSPACE) {
		LInput_Backspace(w);
	} else if (key == KEY_DELETE) {
		LInput_Delete(w);
	} else if (key == INPUT_CLIPBOARD_COPY) {
		if (w->text.length) Clipboard_SetText(&w->text);
	} else if (key == INPUT_CLIPBOARD_PASTE) {
		LInput_CopyFromClipboard(w);
	} else if (key == KEY_ESCAPE) {
		if (w->text.length) LInput_SetString(w, &String_Empty);
	} else if (key == KEY_LEFT) {
		LInput_AdvanceCaretPos(w, false);
	} else if (key == KEY_RIGHT) {
		LInput_AdvanceCaretPos(w, true);
	}
}

static cc_bool LInput_CanAppend(struct LInput* w, char c) {
	switch (w->type) {
	case KEYBOARD_TYPE_PASSWORD:
		return true; /* keyboard accepts all characters */
	case KEYBOARD_TYPE_INTEGER:
		return c >= '0' && c <= '9';
	}
	return c >= ' ' && c <= '~' && c != '&';
}

/* Appends a character to the currently entered text */
static CC_NOINLINE cc_bool LInput_Append(struct LInput* w, char c) {
	if (LInput_CanAppend(w, c) && w->text.length < w->text.capacity) {
		if (w->caretPos == -1) {
			String_Append(&w->text, c);
		} else {
			String_InsertAt(&w->text, w->caretPos, c);
			w->caretPos++;
		}
		return true;
	}
	return false;
}

static void LInput_KeyChar(void* widget, char c) {
	struct LInput* w = (struct LInput*)widget;
	cc_bool appended = LInput_Append(w, c);

	if (appended && w->TextChanged) w->TextChanged(w);
	if (appended) LWidget_Redraw(w);
}

static void LInput_TextChanged(void* widget, const cc_string* str) {
	struct LInput* w = (struct LInput*)widget;
	LInput_SetText(w, str);
	if (w->TextChanged) w->TextChanged(w);
}

static const struct LWidgetVTABLE linput_VTABLE = {
	LInput_Draw, LInput_TickCaret,
	LInput_KeyDown, LInput_KeyChar, /* Key    */
	NULL, NULL,                     /* Hover  */
	/* TODO: Don't redraw whole thing, just the outer border */
	LInput_Select, LInput_Unselect, /* Select */
	NULL, LInput_TextChanged        /* TextChanged */
};
void LInput_Init(struct LInput* w, int width, const char* hintText) {
	w->VTABLE = &linput_VTABLE;
	w->tabSelectable = true;
	String_InitArray(w->text, w->_textBuffer);
	
	w->hintText = hintText;
	w->caretPos = -1;
	LBackend_InputInit(w, width);
	w->minWidth = w->width;
}

void LInput_SetText(struct LInput* w, const cc_string* text) {
	String_Copy(&w->text, text);
	LInput_ClampCaret(w);
	LBackend_InputUpdate(w);
}

void LInput_ClearText(struct LInput* w) {
	w->text.length = 0;
	w->caretPos    = -1;
	LBackend_InputUpdate(w);
}

void LInput_AppendString(struct LInput* w, const cc_string* str) {
	int i, appended = 0;
	for (i = 0; i < str->length; i++) {
		if (LInput_Append(w, str->buffer[i])) appended++;
	}

	if (appended && w->TextChanged) w->TextChanged(w);
	if (appended) LWidget_Redraw(w);
}

void LInput_SetString(struct LInput* w, const cc_string* str) {
	LInput_SetText(w, str);
	if (w->TextChanged) w->TextChanged(w);
	LWidget_Redraw(w);
}


/*########################################################################################################################*
*------------------------------------------------------LabelWidget--------------------------------------------------------*
*#########################################################################################################################*/
static void LLabel_Draw(void* widget) {
	struct LLabel* w = (struct LLabel*)widget;
	LBackend_LabelDraw(w);
}

static const struct LWidgetVTABLE llabel_VTABLE = {
	LLabel_Draw, NULL,
	NULL, NULL, /* Key    */
	NULL, NULL, /* Hover  */
	NULL, NULL  /* Select */
};
void LLabel_Init(struct LLabel* w, const char* text) {
	w->VTABLE = &llabel_VTABLE;
	w->font   = &Launcher_TextFont;

	String_InitArray(w->text, w->_textBuffer);
	LBackend_LabelInit(w);
	LLabel_SetConst(w, text);
}

void LLabel_SetText(struct LLabel* w, const cc_string* text) {
	String_Copy(&w->text, text);
	LBackend_LabelUpdate(w);
	LWidget_CalcPosition(w);
}

void LLabel_SetConst(struct LLabel* w, const char* text) {
	cc_string str = String_FromReadonly(text);
	LLabel_SetText(w, &str);
}


/*########################################################################################################################*
*-------------------------------------------------------LineWidget--------------------------------------------------------*
*#########################################################################################################################*/
static void LLine_Draw(void* widget) {
	struct LLine* w = (struct LLine*)widget;
	LBackend_LineDraw(w);
}

static const struct LWidgetVTABLE lline_VTABLE = {
	LLine_Draw, NULL,
	NULL, NULL, /* Key    */
	NULL, NULL, /* Hover  */
	NULL, NULL  /* Select */
};
void LLine_Init(struct LLine* w, int width) {
	w->VTABLE = &lline_VTABLE;
	LBackend_LineInit(w, width);
}

#define CLASSIC_LINE_COLOR BitmapCol_Make(128,128,128, 255)
BitmapCol LLine_GetColor(void) {
	return Launcher_Theme.ClassicBackground ? CLASSIC_LINE_COLOR : Launcher_Theme.ButtonBorderColor;
}


/*########################################################################################################################*
*------------------------------------------------------SliderWidget-------------------------------------------------------*
*#########################################################################################################################*/
static void LSlider_Draw(void* widget) {
	struct LSlider* w = (struct LSlider*)widget;
	LBackend_SliderDraw(w);
}

static const struct LWidgetVTABLE lslider_VTABLE = {
	LSlider_Draw, NULL,
	NULL, NULL, /* Key    */
	NULL, NULL, /* Hover  */
	NULL, NULL  /* Select */
};
void LSlider_Init(struct LSlider* w, int width, int height, BitmapCol color) {
	w->VTABLE = &lslider_VTABLE;
	w->color  = color;
	LBackend_SliderInit(w, width, height);
}

void LSlider_SetProgress(struct LSlider* w, int progress) {
	if (progress == w->value) return;
	w->value = progress;
	LBackend_SliderUpdate(w);
}


/*########################################################################################################################*
*------------------------------------------------------TableWidget--------------------------------------------------------*
*#########################################################################################################################*/
static void FlagColumn_Draw(struct ServerInfo* row, struct DrawTextArgs* args, struct LTableCell* cell) {
	struct Bitmap* bmp = Flags_Get(row);
	if (!bmp) return;
	Drawer2D_BmpCopy(&Launcher_Framebuffer, cell->x + flagXOffset, cell->y + flagYOffset, bmp);
}

static void NameColumn_Draw(struct ServerInfo* row, struct DrawTextArgs* args, struct LTableCell* cell) {
	args->text = row->name;
}
static int NameColumn_Sort(const struct ServerInfo* a, const struct ServerInfo* b) {
	return String_Compare(&b->name, &a->name);
}

static void PlayersColumn_Draw(struct ServerInfo* row, struct DrawTextArgs* args, struct LTableCell* cell) {
	String_Format2(&args->text, "%i/%i", &row->players, &row->maxPlayers);
}
static int PlayersColumn_Sort(const struct ServerInfo* a, const struct ServerInfo* b) {
	return b->players - a->players;
}

static void UptimeColumn_Draw(struct ServerInfo* row, struct DrawTextArgs* args, struct LTableCell* cell) {
	int uptime = row->uptime;
	char unit  = 's';

	if (uptime >= SECS_PER_DAY * 7) {
		uptime /= SECS_PER_DAY;  unit = 'd';
	} else if (uptime >= SECS_PER_HOUR) {
		uptime /= SECS_PER_HOUR; unit = 'h';
	} else if (uptime >= SECS_PER_MIN) {
		uptime /= SECS_PER_MIN;  unit = 'm';
	}
	String_Format2(&args->text, "%i%r", &uptime, &unit);
}
static int UptimeColumn_Sort(const struct ServerInfo* a, const struct ServerInfo* b) {
	return b->uptime - a->uptime;
}

static void SoftwareColumn_Draw(struct ServerInfo* row, struct DrawTextArgs* args, struct LTableCell* cell) {
	/* last column, so adjust to fit size of table */
	int leftover = cell->table->width - cell->x;
	cell->width  = max(cell->width, leftover);
	args->text   = row->software;
}
static int SoftwareColumn_Sort(const struct ServerInfo* a, const struct ServerInfo* b) {
	return String_Compare(&b->software, &a->software);
}

static struct LTableColumn tableColumns[5] = {
	{ "",          15, FlagColumn_Draw,     NULL,                false, false },
	{ "Name",     328, NameColumn_Draw,     NameColumn_Sort,     true,  true  },
	{ "Players",   73, PlayersColumn_Draw,  PlayersColumn_Sort,  true,  true  },
	{ "Uptime",    73, UptimeColumn_Draw,   UptimeColumn_Sort,   true,  true  },
	{ "Software", 143, SoftwareColumn_Draw, SoftwareColumn_Sort, false, true  }
};


void LTable_GetScrollbarCoords(struct LTable* w, int* y, int* height) {
	float scale;
	if (!w->rowsCount) { *y = 0; *height = 0; return; }

	scale   = w->height / (float)w->rowsCount;
	*y      = Math_Ceil(w->topRow * scale);
	*height = Math_Ceil(w->visibleRows * scale);
	*height = min(*y + *height, w->height) - *y;
}

void LTable_ClampTopRow(struct LTable* w) { 
	if (w->topRow > w->rowsCount - w->visibleRows) {
		w->topRow = w->rowsCount - w->visibleRows;
	}
	if (w->topRow < 0) w->topRow = 0;
}

int LTable_GetSelectedIndex(struct LTable* w) {
	struct ServerInfo* entry;
	int row;

	for (row = 0; row < w->rowsCount; row++) {
		entry = LTable_Get(row);
		if (String_CaselessEquals(w->selectedHash, &entry->hash)) return row;
	}
	return -1;
}

void LTable_SetSelectedTo(struct LTable* w, int index) {
	if (!w->rowsCount) return;
	if (index >= w->rowsCount) index = w->rowsCount - 1;
	if (index < 0) index = 0;

	String_Copy(w->selectedHash, &LTable_Get(index)->hash);
	LTable_ShowSelected(w);
	w->OnSelectedChanged();
}

cc_bool LTable_HandlesKey(int key) {
	return key == KEY_UP || key == KEY_DOWN || key == KEY_PAGEUP || key == KEY_PAGEDOWN;
}

static void LTable_KeyDown(void* widget, int key, cc_bool was) {
	struct LTable* w = (struct LTable*)widget;
	int index = LTable_GetSelectedIndex(w);

	if (key == KEY_UP) {
		index--;
	} else if (key == KEY_DOWN) {
		index++;
	} else if (key == KEY_PAGEUP) {
		index -= w->visibleRows;
	} else if (key == KEY_PAGEDOWN) {
		index += w->visibleRows;
	} else { return; }

	w->_lastRow = -1;
	LTable_SetSelectedTo(w, index);
}

static void LTable_MouseDown(void* widget, int idx, cc_bool wasOver) {
	struct LTable* w = (struct LTable*)widget;
	LBackend_TableMouseDown(w, idx);
}

static void LTable_MouseMove(void* widget, int idx, cc_bool wasOver) {
	struct LTable* w = (struct LTable*)widget;
	LBackend_TableMouseMove(w, idx);
}

static void LTable_MouseUp(void* widget, int idx) {
	struct LTable* w = (struct LTable*)widget;
	LBackend_TableMouseUp(w, idx);
}

static void LTable_MouseWheel(void* widget, float delta) {
	struct LTable* w = (struct LTable*)widget;
	w->topRow -= Utils_AccumulateWheelDelta(&w->_wheelAcc, delta);
	LTable_ClampTopRow(w);
	LWidget_Draw(w);
	w->_lastRow = -1;
}

void LTable_Reposition(struct LTable* w) {
	LBackend_TableReposition(w);
	LTable_ClampTopRow(w);
}

static void LTable_Draw(void* widget) {
	struct LTable* w = (struct LTable*)widget;
	LBackend_TableDraw(w);
}

static const struct LWidgetVTABLE ltable_VTABLE = {
	LTable_Draw,      NULL,
	LTable_KeyDown,   NULL, /* Key    */
	LTable_MouseMove, NULL, /* Hover  */
	LTable_MouseDown, LTable_MouseUp, /* Select */
	LTable_MouseWheel,      /* Wheel */
};
void LTable_Init(struct LTable* w, struct FontDesc* rowFont) {
	int i;
	w->VTABLE     = &ltable_VTABLE;
	w->columns    = tableColumns;
	w->numColumns = Array_Elems(tableColumns);
	w->rowFont    = rowFont;
	w->sortingCol = -1;
	
	for (i = 0; i < w->numColumns; i++) {
		w->columns[i].width = Display_ScaleX(w->columns[i].width);
	}
}

void LTable_Reset(struct LTable* w) {
	LBackend_TableMouseUp(w, 0);
	LTable_Reposition(w);

	w->topRow     = 0;
	w->rowsCount  = 0;
	w->_wheelAcc  = 0.0f;
	w->sortingCol = -1;
}

static int ShouldShowServer(struct LTable* w, struct ServerInfo* server) {
	return String_CaselessContains(&server->name, w->filter) 
		&& (Launcher_ShowEmptyServers || server->players > 0);
}

void LTable_ApplyFilter(struct LTable* w) {
	int i, j, count;

	count = FetchServersTask.numServers;
	for (i = 0, j = 0; i < count; i++) {
		if (ShouldShowServer(w, Servers_Get(i))) {
			FetchServersTask.servers[j++]._order = FetchServersTask.orders[i];
		}
	}

	w->rowsCount = j;
	for (; j < count; j++) {
		FetchServersTask.servers[j]._order = -100000;
	}

	w->_lastRow = -1;
	LTable_ClampTopRow(w);
}

static int sortingCol;
static int LTable_SortOrder(const struct ServerInfo* a, const struct ServerInfo* b) {
	int order;
	if (sortingCol >= 0) {
		order = tableColumns[sortingCol].SortOrder(a, b);
		return tableColumns[sortingCol].invertSort ? -order : order;
	}

	/* Default sort order. (most active server, then by highest uptime) */
	if (a->players != b->players) return a->players - b->players;
	return a->uptime - b->uptime;
}

static void LTable_QuickSort(int left, int right) {
	cc_uint16* keys = FetchServersTask.orders; cc_uint16 key;

	while (left < right) {
		int i = left, j = right;
		struct ServerInfo* mid = Servers_Get((i + j) >> 1);

		/* partition the list */
		while (i <= j) {
			while (LTable_SortOrder(mid, Servers_Get(i)) < 0) i++;
			while (LTable_SortOrder(mid, Servers_Get(j)) > 0) j--;
			QuickSort_Swap_Maybe();
		}
		/* recurse into the smaller subset */
		QuickSort_Recurse(LTable_QuickSort)
	}
}

void LTable_Sort(struct LTable* w) {
	if (!FetchServersTask.numServers) return;

	sortingCol = w->sortingCol;
	FetchServersTask_ResetOrder();
	LTable_QuickSort(0, FetchServersTask.numServers - 1);

	LTable_ApplyFilter(w);
	LTable_ShowSelected(w);
}

void LTable_ShowSelected(struct LTable* w) {
	int i = LTable_GetSelectedIndex(w);
	if (i == -1) return;

	if (i >= w->topRow + w->visibleRows) {
		w->topRow = i - (w->visibleRows - 1);
	}
	if (i < w->topRow) w->topRow = i;
	LTable_ClampTopRow(w);
}
#endif
