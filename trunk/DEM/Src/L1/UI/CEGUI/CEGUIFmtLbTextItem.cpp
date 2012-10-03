#include <StdCfg.h>
#include "CEGUIFmtLbTextItem.h"

#include <CEGUILeftAlignedRenderedString.h>
#include <CEGUIRightAlignedRenderedString.h>
#include <CEGUICentredRenderedString.h>
#include <CEGUIRenderedStringWordWrapper.h>
#include <CEGUIImage.h>
#include <elements/CEGUIListbox.h>

namespace CEGUI
{

//----------------------------------------------------------------------------//
FormattedListboxTextItem::FormattedListboxTextItem(const String& text,
	const HorizontalTextFormatting format,
	const uint item_id,
	void* const item_data,
	const bool disabled,
	const bool auto_delete) :
// initialise base class
ListboxTextItem(text, item_id, item_data, disabled, auto_delete),
	// initialise subclass fields
	d_formatting(format),
	d_formattedRenderedString(NULL),
	d_formattingAreaSize(0, 0)
{
}

//----------------------------------------------------------------------------//
FormattedListboxTextItem::~FormattedListboxTextItem()
{
	delete d_formattedRenderedString;
}

//----------------------------------------------------------------------------//
void FormattedListboxTextItem::setFormatting(const HorizontalTextFormatting fmt)
{
	if (fmt == d_formatting)
		return;

	d_formatting = fmt;
	delete d_formattedRenderedString;
	d_formattedRenderedString = NULL;
	d_formattingAreaSize = Size(0, 0);
}

//----------------------------------------------------------------------------//
void FormattedListboxTextItem::updateString() const
{
	bool StringChanged = !d_renderedStringValid;
	if (StringChanged) parseTextString();

	if (!d_formattedRenderedString) setupStringFormatter();
	else if (StringChanged) d_formattedRenderedString->setRenderedString(d_renderedString);

	// get size of render area from target window, to see if we need to reformat
	// NB: We do not use targetRect, since it may not represent the same area.
	const Size area_sz(static_cast<const Listbox*>(d_owner)->getListRenderArea().getSize());
	if (StringChanged || area_sz != d_formattingAreaSize)
	{
		d_formattedRenderedString->format(area_sz);
		d_formattingAreaSize = area_sz;
	}
}

//----------------------------------------------------------------------------//
Size FormattedListboxTextItem::getPixelSize(void) const
{
	if (!d_owner) return Size(0, 0);

	updateString();

	return Size(d_formattedRenderedString->getHorizontalExtent(),
		d_formattedRenderedString->getVerticalExtent());
}

//----------------------------------------------------------------------------//
void FormattedListboxTextItem::draw(GeometryBuffer& buffer,
	const Rect& targetRect,
	float alpha, const Rect* clipper) const
{
	updateString();

	// draw selection imagery
	if (d_selected && d_selectBrush != 0)
		d_selectBrush->draw(buffer, targetRect, clipper,
		getModulateAlphaColourRect(d_selectCols, alpha));

	// factor the window alpha into our colours.
	const ColourRect final_colours(
		getModulateAlphaColourRect(ColourRect(0xFFFFFFFF), alpha));

	// draw the formatted text
	d_formattedRenderedString->draw(buffer, targetRect.getPosition(),
		&final_colours, clipper);
}

//----------------------------------------------------------------------------//
void FormattedListboxTextItem::setupStringFormatter() const
{
	// delete any existing formatter
	delete d_formattedRenderedString;

	// create new formatter of whichever type...
	switch(d_formatting)
	{
	case HTF_LEFT_ALIGNED:
		d_formattedRenderedString =
			new LeftAlignedRenderedString(d_renderedString);
		break;

	case HTF_RIGHT_ALIGNED:
		d_formattedRenderedString =
			new RightAlignedRenderedString(d_renderedString);
		break;

	case HTF_CENTRE_ALIGNED:
		d_formattedRenderedString =
			new CentredRenderedString(d_renderedString);
		break;

	case HTF_JUSTIFIED:
		d_formattedRenderedString =
			new JustifiedRenderedString(d_renderedString);
		break;

	case HTF_WORDWRAP_LEFT_ALIGNED:
		d_formattedRenderedString =
			new RenderedStringWordWrapper
			<LeftAlignedRenderedString>(d_renderedString);
		break;

	case HTF_WORDWRAP_RIGHT_ALIGNED:
		d_formattedRenderedString =
			new RenderedStringWordWrapper
			<RightAlignedRenderedString>(d_renderedString);
		break;

	case HTF_WORDWRAP_CENTRE_ALIGNED:
		d_formattedRenderedString =
			new RenderedStringWordWrapper
			<CentredRenderedString>(d_renderedString);
		break;

	case HTF_WORDWRAP_JUSTIFIED:
		d_formattedRenderedString =
			new RenderedStringWordWrapper
			<JustifiedRenderedString>(d_renderedString);
		break;

	default: d_formattedRenderedString = NULL; break;
	}
}

//----------------------------------------------------------------------------//

}