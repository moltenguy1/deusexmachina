#pragma once
#ifndef __DEM_L1_CEGUI_FMT_LABEL_TEXT_ITEM_H__
#define __DEM_L1_CEGUI_FMT_LABEL_TEXT_ITEM_H__

#include <elements/CEGUIListboxTextItem.h>

namespace CEGUI
{

//! A ListboxItem based class that can do horizontal text formatiing.
class FormattedListboxTextItem : public ListboxTextItem
{
public:
	//! Constructor
	FormattedListboxTextItem(const String& text,
		const HorizontalTextFormatting format = HTF_LEFT_ALIGNED,
		const uint item_id = 0,
		void* const item_data = 0,
		const bool disabled = false,
		const bool auto_delete = true);

	//! Destructor.
	virtual ~FormattedListboxTextItem();

	//! Return the current formatting set.
	HorizontalTextFormatting getFormatting() const { return d_formatting; }
	/*!
	Set the formatting.  You should call Listbox::handleUpdatedItemData
	after setting the formatting in order to update the listbox.  We do not
	do it automatically since you may wish to batch changes to multiple
	items and multiple calls to handleUpdatedItemData is wasteful.
	*/
	void setFormatting(const HorizontalTextFormatting fmt);

	// overriden functions.
	Size getPixelSize(void) const;
	void draw(GeometryBuffer& buffer, const Rect& targetRect,
		float alpha, const Rect* clipper) const;

protected:
	//! Helper to create a FormattedRenderedString of an appropriate type.
	void updateString() const;
	void setupStringFormatter() const;
	//! Current formatting set
	HorizontalTextFormatting d_formatting;
	//! Class that renders RenderedString with some formatting.
	mutable FormattedRenderedString* d_formattedRenderedString;
	//! Tracks target area for rendering so we can reformat when needed
	mutable Size d_formattingAreaSize;
};

}

#endif
