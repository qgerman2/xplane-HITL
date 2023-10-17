#include <XPLMDataAccess.h>
#include <XPLMUtilities.h>
#include <XPLMDisplay.h>
#include <XPLMGraphics.h>
#include <XPUIGraphics.h>
#include <XPWidgets.h>
#include <XPWidgetUtils.h>
#include <XPWidgetsEx.h>
#include <gl/GL.h>
#include <string>
#include <vector>
/*------------------------------------------------------------------------*/


/*
 * XPListBox.cpp
 *
 * Copyright 2005 Sandy Barbour and Ben Supnik
 *
 * All rights reserved.  See license.txt for usage.
 *
 * X-Plane SDK Version: 1.0.2
 *
 */


#define LISTBOX_ITEM_HEIGHT 12
#define IN_RECT(x, y, l, t, r, b)	\
	(((x) >= (l)) && ((x) <= (r)) && ((y) >= (b)) && ((y) <= (t)))

#if IBM
double round(double InValue) {
    int WholeValue;
    double Fraction;

    WholeValue = InValue;
    Fraction = InValue - (double)WholeValue;

    if (Fraction >= 0.5)
        WholeValue++;

    return (double)WholeValue;
}
#endif

/************************************************************************
 *  LISTBOX DATA IMPLEMENTATION
 ************************************************************************/

 // This structure represents a listbox internally...it consists of arrays
 // per item and some general stuff.
struct	XPListBoxData_t {
    // Per item:
    std::vector<std::string>	Items;		// The name of the item
    std::vector<int>			Lefts;		// The rectangle of the item, relative to the top left corner of the listbox/
    std::vector<int>			Rights;
};

XPListBoxData_t *pListBoxData;

// This routine finds the item that is in a given point, or returns -1 if there is none.
// It simply trolls through all the items.
int XPListBoxGetItemNumber(XPListBoxData_t *pListBoxData, int inX, int inY) {
    for (unsigned int n = 0; n < pListBoxData->Items.size(); ++n) {
        if ((inX >= pListBoxData->Lefts[n]) && (inX < pListBoxData->Rights[n]) &&
            (inY >= (n * LISTBOX_ITEM_HEIGHT)) && (inY < ((n * LISTBOX_ITEM_HEIGHT) + LISTBOX_ITEM_HEIGHT))) {
            return n;
        }
    }
    return -1;
}

void XPListBoxFillWithData(XPListBoxData_t *pListBoxData, const char *inItems, int Width) {
    std::string	Items(inItems);
    while (!Items.empty()) {
        std::string::size_type split = Items.find(';');
        if (split == Items.npos) {
            split = Items.size();
        }

        std::string	Item = Items.substr(0, split);

        pListBoxData->Items.push_back(Item);
        pListBoxData->Lefts.push_back(0);
        pListBoxData->Rights.push_back(Width);

        if (Item.size() == Items.size())
            break;
        else
            Items = Items.substr(split + 1);
    }
}

void XPListBoxAddItem(XPListBoxData_t *pListBoxData, char *pBuffer, int Width) {
    std::string	Item(pBuffer);

    pListBoxData->Items.push_back(Item);
    pListBoxData->Lefts.push_back(0);
    pListBoxData->Rights.push_back(Width);
}

void XPListBoxClear(XPListBoxData_t *pListBoxData) {
    pListBoxData->Items.clear();
    pListBoxData->Lefts.clear();
    pListBoxData->Rights.clear();
}

void XPListBoxInsertItem(XPListBoxData_t *pListBoxData, char *pBuffer, int Width, int CurrentItem) {
    std::string	Item(pBuffer);

    pListBoxData->Items.insert(pListBoxData->Items.begin() + CurrentItem, Item);
    pListBoxData->Lefts.insert(pListBoxData->Lefts.begin() + CurrentItem, 0);
    pListBoxData->Rights.insert(pListBoxData->Rights.begin() + CurrentItem, Width);
}

void XPListBoxDeleteItem(XPListBoxData_t *pListBoxData, int CurrentItem) {
    pListBoxData->Items.erase(pListBoxData->Items.begin() + CurrentItem);
    pListBoxData->Lefts.erase(pListBoxData->Lefts.begin() + CurrentItem);
    pListBoxData->Rights.erase(pListBoxData->Rights.begin() + CurrentItem);
}
// This widget Proc implements the actual listbox.

int		XPListBoxProc(
    XPWidgetMessage			inMessage,
    XPWidgetID				inWidget,
    intptr_t				inParam1,
    intptr_t				inParam2) {
    int ScrollBarSlop = 0;

    // Select if we're in the background.
    if (XPUSelectIfNeeded(inMessage, inWidget, inParam1, inParam2, 1/*eat*/))	return 1;

    int Left, Top, Right, Bottom, x, y, ListBoxDataOffset, ListBoxIndex;
    char Buffer[4096];

    int IsVertical, DownBtnSize, DownPageSize, ThumbSize, UpPageSize, UpBtnSize;
    bool UpBtnSelected, DownBtnSelected, ThumbSelected, UpPageSelected, DownPageSelected;

    XPGetWidgetGeometry(inWidget, &Left, &Top, &Right, &Bottom);

    int	SliderPosition = XPGetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarSliderPosition, NULL);
    int	Min = XPGetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarMin, NULL);
    int	Max = XPGetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarMax, NULL);
    int	ScrollBarPageAmount = XPGetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarPageAmount, NULL);
    int	CurrentItem = XPGetWidgetProperty(inWidget, xpProperty_ListBoxCurrentItem, NULL);
    int	MaxListBoxItems = XPGetWidgetProperty(inWidget, xpProperty_ListBoxMaxListBoxItems, NULL);
    int	Highlighted = XPGetWidgetProperty(inWidget, xpProperty_ListBoxHighlighted, NULL);
    XPListBoxData_t *pListBoxData = (XPListBoxData_t *)XPGetWidgetProperty(inWidget, xpProperty_ListBoxData, NULL);

    switch (inMessage) {
    case xpMsg_Create:
        // Allocate mem for the structure.
        pListBoxData = new XPListBoxData_t;
        XPGetWidgetDescriptor(inWidget, Buffer, sizeof(Buffer));
        XPListBoxFillWithData(pListBoxData, Buffer, (Right - Left - 20));
        XPSetWidgetProperty(inWidget, xpProperty_ListBoxData, (intptr_t)pListBoxData);
        XPSetWidgetProperty(inWidget, xpProperty_ListBoxCurrentItem, 0);
        Min = 0;
        Max = pListBoxData->Items.size();
        ScrollBarSlop = 0;
        Highlighted = false;
        SliderPosition = Max;
        MaxListBoxItems = (Top - Bottom) / LISTBOX_ITEM_HEIGHT;
        ScrollBarPageAmount = MaxListBoxItems;
        XPSetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarSliderPosition, SliderPosition);
        XPSetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarMin, Min);
        XPSetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarMax, Max);
        XPSetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarPageAmount, ScrollBarPageAmount);
        XPSetWidgetProperty(inWidget, xpProperty_ListBoxMaxListBoxItems, MaxListBoxItems);
        XPSetWidgetProperty(inWidget, xpProperty_ListBoxHighlighted, Highlighted);
        return 1;

    case xpMsg_DescriptorChanged:
        return 1;

    case xpMsg_PropertyChanged:
        if (XPGetWidgetProperty(inWidget, xpProperty_ListBoxAddItem, NULL)) {
            XPSetWidgetProperty(inWidget, xpProperty_ListBoxAddItem, 0);
            XPGetWidgetDescriptor(inWidget, Buffer, sizeof(Buffer));
            XPListBoxAddItem(pListBoxData, Buffer, (Right - Left - 20));
            Max = pListBoxData->Items.size();
            SliderPosition = Max;
            XPSetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarSliderPosition, SliderPosition);
            XPSetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarMax, Max);
        }

        if (XPGetWidgetProperty(inWidget, xpProperty_ListBoxAddItemsWithClear, NULL)) {
            XPSetWidgetProperty(inWidget, xpProperty_ListBoxAddItemsWithClear, 0);
            XPGetWidgetDescriptor(inWidget, Buffer, sizeof(Buffer));
            XPListBoxClear(pListBoxData);
            XPListBoxFillWithData(pListBoxData, Buffer, (Right - Left - 20));
            Max = pListBoxData->Items.size();
            SliderPosition = Max;
            XPSetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarSliderPosition, SliderPosition);
            XPSetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarMax, Max);
        }

        if (XPGetWidgetProperty(inWidget, xpProperty_ListBoxClear, NULL)) {
            XPSetWidgetProperty(inWidget, xpProperty_ListBoxClear, 0);
            XPSetWidgetProperty(inWidget, xpProperty_ListBoxCurrentItem, 0);
            XPListBoxClear(pListBoxData);
            Max = pListBoxData->Items.size();
            SliderPosition = Max;
            XPSetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarSliderPosition, SliderPosition);
            XPSetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarMax, Max);
        }

        if (XPGetWidgetProperty(inWidget, xpProperty_ListBoxInsertItem, NULL)) {
            XPSetWidgetProperty(inWidget, xpProperty_ListBoxInsertItem, 0);
            XPGetWidgetDescriptor(inWidget, Buffer, sizeof(Buffer));
            XPListBoxInsertItem(pListBoxData, Buffer, (Right - Left - 20), CurrentItem);
        }

        if (XPGetWidgetProperty(inWidget, xpProperty_ListBoxDeleteItem, NULL)) {
            XPSetWidgetProperty(inWidget, xpProperty_ListBoxDeleteItem, 0);
            if ((pListBoxData->Items.size() > 0) && (pListBoxData->Items.size() > CurrentItem))
                XPListBoxDeleteItem(pListBoxData, CurrentItem);
        }
        return 1;

    case xpMsg_Draw:
    {
        int	x, y;
        XPLMGetMouseLocation(&x, &y);

        XPDrawWindow(Left, Bottom, Right - 20, Top, xpWindow_ListView);
        XPDrawTrack(Right - 20, Bottom, Right, Top, Min, Max, SliderPosition, xpTrack_ScrollBar, Highlighted);

        XPLMSetGraphicsState(0, 1, 0, 0, 1, 0, 0);
        XPLMBindTexture2d(XPLMGetTexture(xplm_Tex_GeneralInterface), 0);
        glColor4f(1.0, 1.0, 1.0, 1.0);

        unsigned int ItemNumber;
        XPLMSetGraphicsState(0, 0, 0, 0, 0, 0, 0);

        // Now draw each item.
        ListBoxIndex = Max - SliderPosition;
        ItemNumber = 0;
        while (ItemNumber < MaxListBoxItems) {
            if (ListBoxIndex < pListBoxData->Items.size()) {
                // Calculate the item rect in global coordinates.
                int ItemTop = Top - (ItemNumber * LISTBOX_ITEM_HEIGHT);
                int ItemBottom = Top - ((ItemNumber * LISTBOX_ITEM_HEIGHT) + LISTBOX_ITEM_HEIGHT);

                // If we are hilited, draw the hilite bkgnd.
                if (CurrentItem == ListBoxIndex) {
                    SetAlphaLevels(0.25);
                    XPLMSetGraphicsState(0, 0, 0, 0, 1, 0, 0);
                    SetupAmbientColor(xpColor_MenuHilite, NULL);
                    SetAlphaLevels(1.0);
                    glBegin(GL_QUADS);
                    glVertex2i(Left, ItemTop);
                    glVertex2i(Right - 20, ItemTop);
                    glVertex2i(Right - 20, ItemBottom);
                    glVertex2i(Left, ItemBottom);
                    glEnd();
                }

                float	text[3];
                SetupAmbientColor(xpColor_ListText, text);

                char	Buffer[512];
                int		FontWidth, FontHeight;
                int		ListBoxWidth = (Right - 20) - Left;
                strcpy(Buffer, pListBoxData->Items[ListBoxIndex++].c_str());
                XPLMGetFontDimensions(xplmFont_Basic, &FontWidth, &FontHeight, NULL);
                int		MaxChars = ListBoxWidth / FontWidth;
                Buffer[MaxChars] = 0;

                XPLMDrawString(text,
                    Left, ItemBottom + 2,
                    const_cast<char *>(Buffer), NULL, xplmFont_Basic);
            }
            ItemNumber++;
        }
    }
    return 1;

    case xpMsg_MouseUp:
        if (IN_RECT(MOUSE_X(inParam1), MOUSE_Y(inParam1), Right - 20, Top, Right, Bottom)) {
            Highlighted = false;
            XPSetWidgetProperty(inWidget, xpProperty_ListBoxHighlighted, Highlighted);
        }

        if (IN_RECT(MOUSE_X(inParam1), MOUSE_Y(inParam1), Left, Top, Right - 20, Bottom)) {
            if (pListBoxData->Items.size() > 0) {
                if (CurrentItem != -1)
                    XPSetWidgetDescriptor(inWidget, pListBoxData->Items[CurrentItem].c_str());
                else
                    XPSetWidgetDescriptor(inWidget, "");
                XPSendMessageToWidget(inWidget, xpMessage_ListBoxItemSelected, xpMode_UpChain, (intptr_t)inWidget, (intptr_t)CurrentItem);
            }
        }
        return 1;

    case xpMsg_MouseDown:
        if (IN_RECT(MOUSE_X(inParam1), MOUSE_Y(inParam1), Left, Top, Right - 20, Bottom)) {
            if (pListBoxData->Items.size() > 0) {
                XPLMGetMouseLocation(&x, &y);
                ListBoxDataOffset = XPListBoxGetItemNumber(pListBoxData, x - Left, Top - y);
                if (ListBoxDataOffset != -1) {
                    ListBoxDataOffset += (Max - SliderPosition);
                    if (ListBoxDataOffset < pListBoxData->Items.size())
                        XPSetWidgetProperty(inWidget, xpProperty_ListBoxCurrentItem, ListBoxDataOffset);
                }
            }
        }

        if (IN_RECT(MOUSE_X(inParam1), MOUSE_Y(inParam1), Right - 20, Top, Right, Bottom)) {
            XPGetTrackMetrics(Right - 20, Bottom, Right, Top, Min, Max, SliderPosition, xpTrack_ScrollBar, &IsVertical, &DownBtnSize, &DownPageSize, &ThumbSize, &UpPageSize, &UpBtnSize);
            int	Min = XPGetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarMin, NULL);
            int	Max = XPGetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarMax, NULL);
            if (IsVertical) {
                UpBtnSelected = IN_RECT(MOUSE_X(inParam1), MOUSE_Y(inParam1), Right - 20, Top, Right, Top - UpBtnSize);
                DownBtnSelected = IN_RECT(MOUSE_X(inParam1), MOUSE_Y(inParam1), Right - 20, Bottom + DownBtnSize, Right, Bottom);
                UpPageSelected = IN_RECT(MOUSE_X(inParam1), MOUSE_Y(inParam1), Right - 20, (Top - UpBtnSize), Right, (Bottom + DownBtnSize + DownPageSize + ThumbSize));
                DownPageSelected = IN_RECT(MOUSE_X(inParam1), MOUSE_Y(inParam1), Right - 20, (Top - UpBtnSize - UpPageSize - ThumbSize), Right, (Bottom + DownBtnSize));
                ThumbSelected = IN_RECT(MOUSE_X(inParam1), MOUSE_Y(inParam1), Right - 20, (Top - UpBtnSize - UpPageSize), Right, (Bottom + DownBtnSize + DownPageSize));
            } else {
                DownBtnSelected = IN_RECT(MOUSE_X(inParam1), MOUSE_Y(inParam1), Right - 20, Top, Right - 20 + UpBtnSize, Bottom);
                UpBtnSelected = IN_RECT(MOUSE_X(inParam1), MOUSE_Y(inParam1), Right - 20 - DownBtnSize, Top, Right, Bottom);
                DownPageSelected = IN_RECT(MOUSE_X(inParam1), MOUSE_Y(inParam1), Right - 20 + DownBtnSize, Top, Right - UpBtnSize - UpPageSize - ThumbSize, Bottom);
                UpPageSelected = IN_RECT(MOUSE_X(inParam1), MOUSE_Y(inParam1), Right - 20 + DownBtnSize + DownPageSize + ThumbSize, Top, Right - UpBtnSize, Bottom);
                ThumbSelected = IN_RECT(MOUSE_X(inParam1), MOUSE_Y(inParam1), Right - 20 + DownBtnSize + DownPageSize, Top, Right - UpBtnSize - UpPageSize, Bottom);
            }

            if (UpPageSelected) {
                SliderPosition += ScrollBarPageAmount;
                if (SliderPosition > Max)
                    SliderPosition = Max;
                XPSetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarSliderPosition, SliderPosition);
            } else if (DownPageSelected) {
                SliderPosition -= ScrollBarPageAmount;
                if (SliderPosition < Min)
                    SliderPosition = Min;
                XPSetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarSliderPosition, SliderPosition);
            } else if (UpBtnSelected) {
                SliderPosition++;
                if (SliderPosition > Max)
                    SliderPosition = Max;
                XPSetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarSliderPosition, SliderPosition);
            } else if (DownBtnSelected) {
                SliderPosition--;
                if (SliderPosition < Min)
                    SliderPosition = Min;
                XPSetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarSliderPosition, SliderPosition);
            } else if (ThumbSelected) {
                if (IsVertical)
                    ScrollBarSlop = Bottom + DownBtnSize + DownPageSize + (ThumbSize / 2) - MOUSE_Y(inParam1);
                else
                    ScrollBarSlop = Right - 20 + DownBtnSize + DownPageSize + (ThumbSize / 2) - MOUSE_X(inParam1);
                Highlighted = true;
                XPSetWidgetProperty(inWidget, xpProperty_ListBoxHighlighted, Highlighted);

            } else {
                Highlighted = false;
                XPSetWidgetProperty(inWidget, xpProperty_ListBoxHighlighted, Highlighted);
            }
        }
        return 1;

    case xpMsg_MouseDrag:
        if (IN_RECT(MOUSE_X(inParam1), MOUSE_Y(inParam1), Right - 20, Top, Right, Bottom)) {
            XPGetTrackMetrics(Right - 20, Bottom, Right, Top, Min, Max, SliderPosition, xpTrack_ScrollBar, &IsVertical, &DownBtnSize, &DownPageSize, &ThumbSize, &UpPageSize, &UpBtnSize);
            int	Min = XPGetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarMin, NULL);
            int	Max = XPGetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarMax, NULL);

            ThumbSelected = Highlighted;

            if (ThumbSelected) {
                if (inParam1 != 0) {
                    if (IsVertical) {
                        y = MOUSE_Y(inParam1) + ScrollBarSlop;
                        SliderPosition = round((float)((float)(y - (Bottom + DownBtnSize + ThumbSize / 2)) /
                            (float)((Top - UpBtnSize - ThumbSize / 2) - (Bottom + DownBtnSize + ThumbSize / 2))) * Max);
                    } else {
                        x = MOUSE_X(inParam1) + ScrollBarSlop;
                        SliderPosition = round((float)((float)(x - (Right - 20 + DownBtnSize + ThumbSize / 2)) / (float)((Right - UpBtnSize - ThumbSize / 2) - (Right - 20 + DownBtnSize + ThumbSize / 2))) * Max);
                    }

                } else
                    SliderPosition = 0;

                if (SliderPosition < Min)
                    SliderPosition = Min;
                if (SliderPosition > Max)
                    SliderPosition = Max;

                XPSetWidgetProperty(inWidget, xpProperty_ListBoxScrollBarSliderPosition, SliderPosition);
            }
        }
        return 1;

    default:
        return 0;
    }
}

// To create a listbox, make a new widget with our listbox proc as the widget proc.
XPWidgetID           XPCreateListBox(
    int                  inLeft,
    int                  inTop,
    int                  inRight,
    int                  inBottom,
    int                  inVisible,
    const char *inDescriptor,
    XPWidgetID           inContainer) {
    return XPCreateCustomWidget(
        inLeft,
        inTop,
        inRight,
        inBottom,
        inVisible,
        inDescriptor,
        0,
        inContainer,
        XPListBoxProc);
}


/*------------------------------------------------------------------------*/