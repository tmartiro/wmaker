/* Icons.c- icon preferences
 *
 *  WPrefs - Window Maker Preferences Program
 *
 *  Copyright (c) 1998-2003 Alfredo K. Kojima
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "WPrefs.h"


static const struct {
	const char *db_value;
	const char *label;
} icon_animation[] = {
	{ "zoom",  N_("Shrinking/Zooming") },
	{ "twist", N_("Spinning/Twisting") },
	{ "flip",  N_("3D-flipping") },
	{ "none",  N_("None") }
};

/*
 * The code is using a convenient trick to make the link between the icon
 * position and its representing number:
 *   bit[0] tell if the icon are arranged horizontally or vertically
 *   bit[2:1] tell the corner to which they are starting from:
 *     bit[2] is for Top or Bottom choice
 *     bit[1] is for Left or Right choice
 */
static const char icon_position_dbvalue[][4] = {
	/* 000: */ "tlv",
	/* 001: */ "tlh",
	/* 010: */ "trv",
	/* 011: */ "trh",
	/* 100: */ "blv",
	/* 101: */ "blh",
	/* 110: */ "brv",
	/* 111: */ "brh"
};

typedef struct _Panel {
	WMBox *box;

	char *sectionName;

	char *description;

	CallbackRec callbacks;

	WMWidget *parent;

	WMFrame *posF;
	WMFrame *posVF;
	WMFrame *posV;

	struct {
		int width, height;
	} icon_position;

	WMButton *posB[wlengthof_nocheck(icon_position_dbvalue)];

	WMFrame *animF;
	WMButton *animB[wlengthof_nocheck(icon_animation)];

	WMFrame *optF;
	WMButton *arrB;
	WMButton *omnB;
	WMButton *sclB;

	struct {
		WMFrame *frame;
		WMSlider *slider;
		WMLabel *label;
	} apercu;

	WMFrame *sizeF;
	WMPopUpButton *sizeP;

	int iconPos;
} _Panel;

/*
 * Minimum size for a Mini-Preview:
 * This value is actually twice the size of the minimum icon size choosable.
 * We set the slider min to taht number minus one, because when set to this
 * value WPrefs will consider that the user wants the feature turned off.
 */
static const int apercu_minimum_size = 2 * 24 - 1;

static const int apercu_maximum_size = 512;	/* Arbitrary limit for the slider */

#define ICON_FILE	"iconprefs"

static void showIconLayout(WMWidget * widget, void *data)
{
	_Panel *panel = (_Panel *) data;
	int w, h;
	int i;

	for (i = 0; i < wlengthof(panel->posB); i++) {
		if (panel->posB[i] == widget) {
			panel->iconPos = i;
			break;
		}
	}

	if (panel->iconPos & 1) {
		w = 32;
		h = 8;
	} else {
		w = 8;
		h = 32;
	}
	WMResizeWidget(panel->posV, w, h);

	switch (panel->iconPos & ~1) {
	case 0:
		WMMoveWidget(panel->posV, 2, 2);
		break;
	case 2:
		WMMoveWidget(panel->posV, panel->icon_position.width - 2 - w, 2);
		break;
	case 4:
		WMMoveWidget(panel->posV, 2, panel->icon_position.height - 2 - h);
		break;
	default:
		WMMoveWidget(panel->posV, panel->icon_position.width - 2 - w, panel->icon_position.height - 2 - h);
		break;
	}
}

static void apercu_slider_changed(WMWidget *w, void *data)
{
	_Panel *panel = (_Panel *) data;
	char buffer[64];
	int value;

	/* Parameter is not used, but tell the compiler that it is ok */
	(void) w;

	value = WMGetSliderValue(panel->apercu.slider);

	/* Round the value to a multiple of 8 because it makes the displayed value look better */
	value &= ~7;

	if (value <= apercu_minimum_size)
		sprintf(buffer, _("OFF"));
	else
		sprintf(buffer, "%i", value);

	WMSetLabelText(panel->apercu.label, buffer);
}

static void showData(_Panel * panel)
{
	int i;
	char *str;
	Bool b;

	WMSetButtonSelected(panel->arrB, GetBoolForKey("AutoArrangeIcons"));
	WMSetButtonSelected(panel->omnB, GetBoolForKey("StickyIcons"));
	WMSetButtonSelected(panel->sclB, GetBoolForKey("SingleClickLaunch"));

	str = GetStringForKey("IconPosition");
	if (str != NULL) {
		for (i = 0; i < wlengthof(icon_position_dbvalue); i++)
			if (strcmp(str, icon_position_dbvalue[i]) == 0) {
				panel->iconPos = i;
				goto found_position_value;
			}
		wwarning(_("bad value \"%s\" for option %s, using default \"%s\""),
		         str, "IconPosition", icon_position_dbvalue[5]);
	}
	panel->iconPos = 5;
 found_position_value:
	WMPerformButtonClick(panel->posB[panel->iconPos]);

	i = GetIntegerForKey("IconSize");
	i = (i - 24) / 8;

	if (i < 0)
		i = 0;
	else if (i > 9)
		i = 9;
	WMSetPopUpButtonSelectedItem(panel->sizeP, i);

	/* Mini-Previews for Icons */
	b = GetBoolForKey("MiniwindowApercuBalloons");
	if (b) {
		i = GetIntegerForKey("ApercuSize");
		if (i <= apercu_minimum_size)
			i = apercu_minimum_size;
	} else {
		i = apercu_minimum_size;
	}
	WMSetSliderValue(panel->apercu.slider, i);
	apercu_slider_changed(panel->apercu.slider, panel);

	/* Animation */
	str = GetStringForKey("IconificationStyle");
	if (str != NULL) {
		for (i = 0; i < wlengthof(icon_animation); i++) {
			if (strcasecmp(str, icon_animation[i].db_value) == 0) {
				WMPerformButtonClick(panel->animB[i]);
				goto found_animation_value;
			}
		}
		wwarning(_("animation style \"%s\" is unknown, resetting to \"%s\""),
		         str, icon_animation[0].db_value);
	}
	/* If we're here, no valid value have been found so we fall-back to the default */
	WMPerformButtonClick(panel->animB[0]);
 found_animation_value:
	;
}

static void createPanel(Panel * p)
{
	_Panel *panel = (_Panel *) p;
	WMScreen *scr;
	WMColor *color;
	int i;
	char buf[16];
	int swidth, sheight;
	int width, height;
	int startx, starty;

	panel->box = WMCreateBox(panel->parent);
	WMSetViewExpandsToParent(WMWidgetView(panel->box), 2, 2, 2, 2);

    /***************** Positioning of Icons *****************/
	panel->posF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->posF, 268, 155);
	WMMoveWidget(panel->posF, 12, 6);
	WMSetFrameTitle(panel->posF, _("Icon Positioning"));

	/*
	 * There is an available area of 240 x 122, starting at x=14 y=20
	 * We have to keep 14 pixels on each side for the buttons,
	 * and an extra pixel for spacing. We also want the final dimension
	 * to be an even number so we can have the 2 buttons per side of
	 * the same size.
	 * In this area, we want to have a rectangle with the same aspect
	 * ratio as the screen.
	 */
	scr = WMWidgetScreen(panel->parent);
	swidth = WidthOfScreen(DefaultScreenOfDisplay(WMScreenDisplay(scr)));
	sheight = HeightOfScreen(DefaultScreenOfDisplay(WMScreenDisplay(scr)));

	width = swidth * (122 - 15 * 2) / sheight;
	if (width <= (240 - 15 * 2)) {
		height = 122 - 15 * 2;
	} else {
		width = 240 - 15 * 2;
		height = sheight * (240 - 15 * 2) / swidth;
	}

	panel->icon_position.width  = width;
	panel->icon_position.height = height;

	startx = 14 + (240 - 15 * 2 - width) / 2;
	starty = 20 + (122 - 15 * 2 - height) / 2;

	for (i = 0; i < wlengthof(icon_position_dbvalue); i++) {
		int x, y, w, h;

		panel->posB[i] = WMCreateButton(panel->posF, WBTOnOff);
		WMSetButtonAction(panel->posB[i], showIconLayout, panel);

		if (i > 0)
			WMGroupButtons(panel->posB[0], panel->posB[i]);

		if (i & 1) { /* 0=Vertical, 1=Horizontal */
			w = width / 2;
			h = 14;
		} else {
			w = 14;
			h = height / 2;
		}
		WMResizeWidget(panel->posB[i], w, h);

		x = startx;
		y = starty;
		switch (i) {
		case 0: x +=  0;         y += 15;          break;
		case 1: x += 15;         y +=  0;          break;
		case 2: x += 15 + width; y += 15;          break;
		case 3: x += 15 + w;     y +=  0;          break;
		case 4: x +=  0;         y += 15 + h;      break;
		case 5: x += 15;         y += 15 + height; break;
		case 6: x += 15 + width; y += 15 + h;      break;
		case 7: x += 15 + w;     y += 15 + height; break;
		}
		WMMoveWidget(panel->posB[i], x, y);
	}

	color = WMCreateRGBColor(WMWidgetScreen(panel->parent), 0x5100, 0x5100, 0x7100, True);
	panel->posVF = WMCreateFrame(panel->posF);
	WMResizeWidget(panel->posVF, width, height);
	WMMoveWidget(panel->posVF, startx + 15, starty + 15);
	WMSetFrameRelief(panel->posVF, WRSunken);
	WMSetWidgetBackgroundColor(panel->posVF, color);
	WMReleaseColor(color);

	panel->posV = WMCreateFrame(panel->posVF);
	WMSetFrameRelief(panel->posV, WRSimple);

	WMMapSubwidgets(panel->posF);

    /***************** Icon Size ****************/
	panel->sizeF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->sizeF, 100, 52);
	WMMoveWidget(panel->sizeF, 12, 168);
	WMSetFrameTitle(panel->sizeF, _("Icon Size"));

	WMSetBalloonTextForView(_("The size of the dock/application icon and miniwindows"),
				WMWidgetView(panel->sizeF));

	panel->sizeP = WMCreatePopUpButton(panel->sizeF);
	WMResizeWidget(panel->sizeP, 80, 20);
	WMMoveWidget(panel->sizeP, 10, 19);
	for (i = 24; i <= 96; i += 8) {
		sprintf(buf, "%ix%i", i, i);
		WMAddPopUpButtonItem(panel->sizeP, buf);
	}

	WMMapSubwidgets(panel->sizeF);

	/***************** Mini-Previews ****************/
	panel->apercu.frame = WMCreateFrame(panel->box);
	WMResizeWidget(panel->apercu.frame, 156, 52);
	WMMoveWidget(panel->apercu.frame, 124, 168);
	WMSetFrameTitle(panel->apercu.frame, _("Mini-Previews for Icons"));

	WMSetBalloonTextForView(_("The Mini-Preview provides a small view of the content of the\n"
	                          "window when the mouse is placed over the icon."),
	                        WMWidgetView(panel->apercu.frame));

	panel->apercu.slider = WMCreateSlider(panel->apercu.frame);
	WMResizeWidget(panel->apercu.slider, 109, 15);
	WMMoveWidget(panel->apercu.slider, 11, 23);
	WMSetSliderMinValue(panel->apercu.slider, apercu_minimum_size);
	WMSetSliderMaxValue(panel->apercu.slider, apercu_maximum_size);
	WMSetSliderAction(panel->apercu.slider, apercu_slider_changed, panel);

	panel->apercu.label = WMCreateLabel(panel->apercu.frame);
	WMResizeWidget(panel->apercu.label, 33, 15);
	WMMoveWidget(panel->apercu.label, 120, 23);
	WMSetLabelText(panel->apercu.label, _("OFF"));

	WMMapSubwidgets(panel->apercu.frame);

    /***************** Animation ****************/
	panel->animF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->animF, 215, 110);
	WMMoveWidget(panel->animF, 292, 6);
	WMSetFrameTitle(panel->animF, _("Iconification Animation"));

	for (i = 0; i < wlengthof(icon_animation); i++) {
		panel->animB[i] = WMCreateRadioButton(panel->animF);
		WMResizeWidget(panel->animB[i], 192, 20);
		WMMoveWidget(panel->animB[i], 12, 16 + i * 22);

		if (i > 0)
			WMGroupButtons(panel->animB[0], panel->animB[i]);

		WMSetButtonText(panel->animB[i], _(icon_animation[i].label));
	}

	WMMapSubwidgets(panel->animF);

    /***************** Options ****************/
	panel->optF = WMCreateFrame(panel->box);
	WMResizeWidget(panel->optF, 215, 90);
	WMMoveWidget(panel->optF, 292, 130);
	/*    WMSetFrameTitle(panel->optF, _("Icon Display")); */

	panel->arrB = WMCreateSwitchButton(panel->optF);
	WMResizeWidget(panel->arrB, 198, 20);
	WMMoveWidget(panel->arrB, 12, 10);
	WMSetButtonText(panel->arrB, _("Auto-arrange icons"));

	WMSetBalloonTextForView(_("Keep icons and miniwindows arranged all the time."), WMWidgetView(panel->arrB));

	panel->omnB = WMCreateSwitchButton(panel->optF);
	WMResizeWidget(panel->omnB, 198, 20);
	WMMoveWidget(panel->omnB, 12, 35);
	WMSetButtonText(panel->omnB, _("Omnipresent miniwindows"));

	WMSetBalloonTextForView(_("Make miniwindows be present in all workspaces."), WMWidgetView(panel->omnB));

	panel->sclB = WMCreateSwitchButton(panel->optF);
	WMResizeWidget(panel->sclB, 198, 28);
	WMMoveWidget(panel->sclB, 12, 56);
	WMSetButtonText(panel->sclB, _("Single click activation"));

	WMSetBalloonTextForView(_("Launch applications and restore windows with a single click."), WMWidgetView(panel->sclB));

	WMMapSubwidgets(panel->optF);

	WMRealizeWidget(panel->box);
	WMMapSubwidgets(panel->box);

	showData(panel);
}

static void storeData(_Panel * panel)
{
	int i;

	SetBoolForKey(WMGetButtonSelected(panel->arrB), "AutoArrangeIcons");
	SetBoolForKey(WMGetButtonSelected(panel->omnB), "StickyIcons");
	SetBoolForKey(WMGetButtonSelected(panel->sclB), "SingleClickLaunch");

	SetIntegerForKey(WMGetPopUpButtonSelectedItem(panel->sizeP) * 8 + 24, "IconSize");

	SetStringForKey(icon_position_dbvalue[panel->iconPos], "IconPosition");

	i = WMGetSliderValue(panel->apercu.slider);
	if (i <= apercu_minimum_size) {
		SetBoolForKey(False, "MiniwindowApercuBalloons");
	} else {
		SetBoolForKey(True, "MiniwindowApercuBalloons");
		if (i < apercu_maximum_size) {
			/*
			 * If the value is bigger, it means it was edited by the user manually
			 * so we keep as-is. Otherwise, we round it to a multiple of 8 like it
			 * was done for display
			 */
			i &= ~7;
		}
		SetIntegerForKey(i, "ApercuSize");
	}

	for (i = 0; i < wlengthof(icon_animation); i++) {
		if (WMGetButtonSelected(panel->animB[i])) {
			SetStringForKey(icon_animation[i].db_value, "IconificationStyle");
			break;
		}
	}
}

Panel *InitIcons(WMWidget *parent)
{
	_Panel *panel;

	panel = wmalloc(sizeof(_Panel));

	panel->sectionName = _("Icon Preferences");

	panel->description = _("Icon/Miniwindow handling options. Icon positioning\n"
			       "area, sizes of icons, miniaturization animation style.");

	panel->parent = parent;

	panel->callbacks.createWidgets = createPanel;
	panel->callbacks.updateDomain = storeData;

	AddSection(panel, ICON_FILE);

	return panel;
}
