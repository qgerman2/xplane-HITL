#include <XPLMDataAccess.h>
#include <XPWidgets.h>
#include <gl/GL.h>
/************************************************************************
 *  X-PLANE UI INFRASTRUCTURE CODE
 ************************************************************************
 *
 * This code helps provde an x-plane compatible look.  It is copied from
 * the source code from the widgets DLL; someday listyboxes will be part of
 * this, so our listboxes are written off of the same APIs.
 *
 */

 // Enums for x-plane native colors. 
enum {

    xpColor_MenuDarkTinge = 0,
    xpColor_MenuBkgnd,
    xpColor_MenuHilite,
    xpColor_MenuLiteTinge,
    xpColor_MenuText,
    xpColor_MenuTextDisabled,
    xpColor_SubTitleText,
    xpColor_TabFront,
    xpColor_TabBack,
    xpColor_CaptionText,
    xpColor_ListText,
    xpColor_GlassText,
    xpColor_Count
};

// Enums for the datarefs we get them from.
inline const char *kXPlaneColorNames[] = {
    "sim/graphics/colors/menu_dark_rgb",
    "sim/graphics/colors/menu_bkgnd_rgb",
    "sim/graphics/colors/menu_hilite_rgb",
    "sim/graphics/colors/menu_lite_rgb",
    "sim/graphics/colors/menu_text_rgb",
    "sim/graphics/colors/menu_text_disabled_rgb",
    "sim/graphics/colors/subtitle_text_rgb",
    "sim/graphics/colors/tab_front_rgb",
    "sim/graphics/colors/tab_back_rgb",
    "sim/graphics/colors/caption_text_rgb",
    "sim/graphics/colors/list_text_rgb",
    "sim/graphics/colors/glass_text_rgb"
};

// Those datarefs are only XP7; if we can't find one,
// fall back to this table of X-Plane 6 colors.
inline const float	kBackupColors[xpColor_Count][3] =
{
     { (const float)(33.0 / 256.0), (const float)(41.0 / 256.0), (const float)(44.0 / 256.0) },
     { (const float)(53.0 / 256.0), (const float)(64.0 / 256.0), (const float)(68.0 / 256.0) },
     { (const float)(65.0 / 256.0), (const float)(83.0 / 256.0), (const float)(89.0 / 256.0) },
     { (const float)(65.0 / 256.0), (const float)(83.0 / 256.0), (const float)(89.0 / 256.0) },
     { (const float)0.8, (const float)0.8, (const float)0.8 },
     { (const float)0.4, (const float)0.4, (const float)0.4 }
};

// This array contains the resolved datarefs
inline XPLMDataRef	gColorRefs[xpColor_Count];

// Current alpha levels to blit at.
inline float		gAlphaLevel = 1.0;

// This routine sets up a color from the above table.  Pass
// in a float[3] to get the color; pass in NULL to have the
// OpenGL color be set immediately.
inline void	SetupAmbientColor(int inColorID, float *outColor) {
    // If we're running the first time, resolve all of our datarefs just once.
    static	bool	firstTime = true;
    if (firstTime) {
        firstTime = false;
        for (int n = 0; n < xpColor_Count; ++n) {
            gColorRefs[n] = XPLMFindDataRef(kXPlaneColorNames[n]);
        }
    }

    // If being asked to set the color immediately, allocate some storage.
    float	theColor[4];
    float *target = outColor ? outColor : theColor;

    // If we have a dataref, just fetch the color from the ref.
    if (gColorRefs[inColorID])
        XPLMGetDatavf(gColorRefs[inColorID], target, 0, 3);
    else {

        // If we didn't have a dataref, fetch the ambient cabin lighting,
        // since XP6 dims the UI with night.
        static	XPLMDataRef	ambient_r = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_r");
        static	XPLMDataRef	ambient_g = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_g");
        static	XPLMDataRef	ambient_b = XPLMFindDataRef("sim/graphics/misc/cockpit_light_level_b");

        // Use a backup color but dim it.
        target[0] = kBackupColors[inColorID][0] * XPLMGetDataf(ambient_r);
        target[1] = kBackupColors[inColorID][1] * XPLMGetDataf(ambient_g);
        target[2] = kBackupColors[inColorID][2] * XPLMGetDataf(ambient_b);
    }

    // If the user passed NULL, set the color now using the alpha level.
    if (!outColor) {
        theColor[3] = gAlphaLevel;
        glColor4fv(theColor);
    }
}

// Just remember alpha levels for later.
inline void	SetAlphaLevels(float inAlphaLevel) {
    gAlphaLevel = inAlphaLevel;
}
/*------------------------------------------------------------------------*/

/*
 * XPWidgetsEx.h
 *
 * Copyright 2005 Sandy Barbour and Ben Supnik
 *
 * All rights reserved.  See license.txt for usage.
 *
 * X-Plane SDK Version: 1.0.2
 *
 */

 /************************************************************************
  * POPUP MENU PICKS
  ************************************************************************
  *
  * This code helps do a popup menu item pick.  Since x-plane must be
  * running to track the mouse, this popup menu pick is effectively
  * asynchronous and non-modal to the code...you call the function and
  * some time later your callback is called.
  *
  * However, due to the way the popup pick is structured, it will appear to
  * be somewhat modal to the user in that the next click after the popup
  * is called must belong to it.
  *
  */

  /*
   * XPPopupPick_f
   *
   * This function is called when your popup is picked.  inChoice will be the number
   * of the item picked, or -1 if no item was picked.  (You should almost always ignore
   * a -1.
   *
   */
typedef	void (*XPPopupPick_f)(int inChoice, void *inRefcon);

/*
 * XPPickPopup
 *
 * This routine creates a dynamic 'popup menu' on the fly.  If inCurrentItem is
 * non-negative, it is the item that will be under the mouse.  In this case, the
 * mouse X and Y should be the top left of a popup box if there is such a thing.
 * If inCurrentItem is -1, the popup menu appears at exactly inMouseX and inMouseY.
 *
 * You pass in the items, newline terminated ('\n') as well as a callback that is
 * called when an item is picked, and a ref con for that function.
 *
 */
void		XPPickPopup(
    int				inMouseX,
    int				inMouseY,
    const char *inItems,
    int				inCurrentItem,
    XPPopupPick_f	inCallback,
    void *inRefcon);

/* Impl notes: we can dispose from the mouse up.  So...on mouse up
 * we first call the popup func but then we nuke ourselves.  Internally
 * there is a data structure that is in the refcon of the xplm window that
 * contains the callback for the user and the text, etc. */

 /************************************************************************
  * POPUP MENU BUTTON WIDGET
  ************************************************************************
  *
  * This widget implements a stanard pick-one-from-many-style popup menu
  * button.  The text is taken from the current item.  The descriptor is
  * the items, newline-terminated.
  *
  * A message is sent whenever a new item is picked by the user.
  *
  */

#define	xpWidgetClass_Popup					9

enum {
    // This is the item number of the current item, starting at 0.
    xpProperty_PopupCurrentItem = 1800
};

enum {
    // This message is sent when an item is picked.
    // param 1 is the widget that was picked, param 2
    // is the item number.
    xpMessage_PopupNewItemPicked = 1800
};

/*
 * XPCreatePopup
 *
 * This routine makes a popup widget for you.  You must provide
 * a container for this widget, like a window for it to sit in.
 *
 */
XPWidgetID           XPCreatePopup(
    int                  inLeft,
    int                  inTop,
    int                  inRight,
    int                  inBottom,
    int                  inVisible,
    const char *inDescriptor,
    XPWidgetID           inContainer);

int		XPPopupButtonProc(
    XPWidgetMessage			inMessage,
    XPWidgetID				inWidget,
    intptr_t				inParam1,
    intptr_t				inParam2);


/************************************************************************
 * LISTBOX
 ************************************************************************
 *
 * This code helps do a listbox.  Since x-plane must be
 * running to track the mouse, this listbox is effectively
 * asynchronous and non-modal to the code...you call the function and
 * some time later your callback is called.
 *
 * However, due to the way the listbox is structured, it will appear to
 * be somewhat modal to the user in that the next click after the listbox
 * is called must belong to it.
 *
 */

 /************************************************************************
  * LISTBOX SELECTION WIDGET
  ************************************************************************
  *
  * This widget implements a standard pick-one-from-many-style selection menu
  * button.  The text is taken from the current item.  The descriptor is
  * the items, newline-terminated.
  *
  * A message is sent whenever a new item is picked by the user.
  *
  */

#define	xpWidgetClass_ListBox					10

enum {
    // This is the item number of the current item, starting at 0.
    xpProperty_ListBoxCurrentItem = 1900,
    // This will add an item to the list box at the end.
    xpProperty_ListBoxAddItem = 1901,
    // This will clear the list box and then add the items.
    xpProperty_ListBoxAddItemsWithClear = 1902,
    // This will clear the list box.
    xpProperty_ListBoxClear = 1903,
    // This will insert an item into the list box at the index.
    xpProperty_ListBoxInsertItem = 1904,
    // This will delete an item from the list box at the index.
    xpProperty_ListBoxDeleteItem = 1905,
    // This stores the pointer to the listbox data.
    xpProperty_ListBoxData = 1906,
    // This stores the max Listbox Items.
    xpProperty_ListBoxMaxListBoxItems = 1907,
    // This stores the highlight state.
    xpProperty_ListBoxHighlighted = 1908,
    // This stores the scrollbar Min.
    xpProperty_ListBoxScrollBarMin = 1909,
    // This stores the scrollbar Max.
    xpProperty_ListBoxScrollBarMax = 1910,
    // This stores the scrollbar SliderPosition.
    xpProperty_ListBoxScrollBarSliderPosition = 1911,
    // This stores the scrollbar ScrollBarPageAmount.
    xpProperty_ListBoxScrollBarPageAmount = 1912
};

enum {
    // This message is sent when an item is picked.
    // param 1 is the widget that was picked, param 2
    // is the item number.
    xpMessage_ListBoxItemSelected = 1900
};

/*
 * XPCreateListBox
 *
 * This routine makes a listbox widget for you.  You must provide
 * a container for this widget, like a window for it to sit in.
 *
 */
XPWidgetID           XPCreateListBox(
    int                  inLeft,
    int                  inTop,
    int                  inRight,
    int                  inBottom,
    int                  inVisible,
    const char *inDescriptor,
    XPWidgetID           inContainer);

int		XPListBoxProc(
    XPWidgetMessage			inMessage,
    XPWidgetID				inWidget,
    intptr_t				inParam1,
    intptr_t				inParam2);

/*------------------------------------------------------------------------*/