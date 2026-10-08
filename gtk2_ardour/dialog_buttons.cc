/*
 * Copyright (C) 2026 The Ardour Authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <vector>

#include <ytkmm/button.h>
#include <ytkmm/container.h>
#include <ytkmm/stock.h>

#include "dialog_buttons.h"

#include "pbd/i18n.h"

using namespace Gtk;

void
ArdourDialogButtons::plain_buttons (Container& area)
{
	std::vector<Widget*> children = area.get_children ();

	for (auto const& w : children) {
		Button* b = dynamic_cast<Button*> (w);
		if (!b) {
			Container* c = dynamic_cast<Container*> (w);
			if (c) {
				plain_buttons (*c);
			}
			continue;
		}
		if (!b->get_use_stock ()) {
			continue;
		}
		StockItem item;
		if (!Stock::lookup (StockID (b->get_label ()), item)) {
			continue;
		}
		/* without use-stock the button is rebuilt from the label alone,
		 * and GTK drops the stock image */
		b->set_use_stock (false);
		b->set_use_underline (true);
		b->set_label (item.get_label ());
	}
}

void
ArdourDialogButtons::set_primary (Button& button)
{
	/* see the "primary_dialog_button" style in clearlooks.rc.in */
	button.set_name (X_("PrimaryDialogButton"));
}
