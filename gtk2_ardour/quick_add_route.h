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

#pragma once

#include <ytkmm/adjustment.h>
#include <ytkmm/box.h>
#include <ytkmm/checkbutton.h>
#include <ytkmm/entry.h>
#include <ytkmm/label.h>
#include <ytkmm/spinbutton.h>
#include <ytkmm/table.h>

#include "ardour/types.h"

#include "widgets/ardour_button.h"
#include "widgets/ardour_dropdown.h"

#include "ardour_window.h"
#include "instrument_selector.h"

/** A small, non-modal popover for the common case of adding tracks:
 * type, name, count, channels, input and arm-for-recording. Everything else
 * is one click away via "More options...", which opens the full
 * Add Track/Bus/VCA dialog.
 */
class QuickAddRouteWindow : public ArdourWindow
{
public:
	QuickAddRouteWindow ();

	/** show the popover with its top-left corner at the given root coordinates */
	void popup_at (int root_x, int root_y);

protected:
	bool on_key_press_event (GdkEventKey*);
	bool on_focus_out_event (GdkEventFocus*);

private:
	enum Kind {
		AudioTrack,
		MidiTrack,
		AudioBus
	};

	void set_kind (Kind);
	void add_clicked ();
	void more_clicked ();
	void update_sensitivity ();
	void set_channels (bool stereo);

	/* "Record from" input picker */
	enum {
		InputAuto = -2, ///< follow the input auto-connect preference
		InputNone = -1  ///< leave inputs unconnected
	};
	void refill_inputs ();
	void set_input (int);
	std::string input_label (int) const;
	void connect_inputs (ARDOUR::RouteList const&);

	Kind _kind;

	ArdourWidgets::ArdourButton _audio_button;
	ArdourWidgets::ArdourButton _midi_button;
	ArdourWidgets::ArdourButton _bus_button;

	ArdourWidgets::ArdourButton _mono_button;
	ArdourWidgets::ArdourButton _stereo_button;

	Gtk::Label          _title;
	Gtk::Label          _name_label;
	Gtk::Label          _count_label;
	Gtk::Label          _channels_label;
	Gtk::Label          _instrument_label;
	Gtk::Label          _input_label;
	Gtk::Entry          _name_entry;
	Gtk::Adjustment     _count_adj;
	Gtk::SpinButton     _count_spin;
	Gtk::CheckButton    _arm_button;
	InstrumentSelector  _instrument;
	Gtk::HBox           _channels_box;

	ArdourWidgets::ArdourDropdown _input_selector;
	std::vector<std::string>      _phys_inputs; ///< physical capture ports of the current data type
	int                           _input; ///< InputAuto, InputNone or index of the first port in _phys_inputs

	ArdourWidgets::ArdourButton _more_button;
	ArdourWidgets::ArdourButton _add_button;
};
