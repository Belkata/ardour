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

#include <ydk/gdkkeysyms.h>

#include <ytkmm/alignment.h>
#include <ytkmm/frame.h>
#include <ytkmm/separator.h>

#include "pbd/error.h"

#include "ardour/audio_track.h"
#include "ardour/midi_track.h"
#include "ardour/profile.h"
#include "ardour/rc_configuration.h"
#include "ardour/session.h"

#include "widgets/tooltips.h"

#include "ardour_ui.h"
#include "quick_add_route.h"
#include "ui_config.h"

#include "pbd/i18n.h"

using namespace ARDOUR;
using namespace ArdourWidgets;
using namespace Gtk;
using namespace PBD;
using std::string;

QuickAddRouteWindow::QuickAddRouteWindow ()
	: ArdourWindow (_("Add Track"))
	, _kind (AudioTrack)
	, _audio_button (_("Audio"))
	, _midi_button (_("MIDI"))
	, _bus_button (_("Bus"))
	, _mono_button (_("Mono"))
	, _stereo_button (_("Stereo"))
	, _count_adj (1, 1, 128, 1, 4)
	, _count_spin (_count_adj)
	, _arm_button (_("Arm for recording"))
	, _instrument (InstrumentSelector::ForTrackSelector)
	, _more_button (_("More options…"))
	, _add_button (_("Add Track"))
{
	set_decorated (false);
	set_skip_taskbar_hint (true);
	set_skip_pager_hint (true);
	set_resizable (false);
	set_type_hint (Gdk::WINDOW_TYPE_HINT_DIALOG);

	_title.set_markup (string_compose ("<b>%1</b>", _("Add Track")));
	_title.set_alignment (0, 0.5);

	/* type: segmented Audio | MIDI | Bus */
	ArdourButton* kinds[] = { &_audio_button, &_midi_button, &_bus_button };
	HBox* kind_box = manage (new HBox (true, 1));
	for (int i = 0; i < 3; ++i) {
		kinds[i]->set_name ("page switch button");
		kinds[i]->set_corner_mask (i == 0 ? ArdourButton::LEFT : (i == 2 ? ArdourButton::RIGHT : ArdourButton::NONE));
		kind_box->pack_start (*kinds[i], true, true);
	}
	_audio_button.signal_clicked.connect (sigc::bind (sigc::mem_fun (*this, &QuickAddRouteWindow::set_kind), AudioTrack));
	_midi_button.signal_clicked.connect (sigc::bind (sigc::mem_fun (*this, &QuickAddRouteWindow::set_kind), MidiTrack));
	_bus_button.signal_clicked.connect (sigc::bind (sigc::mem_fun (*this, &QuickAddRouteWindow::set_kind), AudioBus));

	/* channels: segmented Mono | Stereo */
	_mono_button.set_name ("page switch button");
	_stereo_button.set_name ("page switch button");
	_mono_button.set_corner_mask (ArdourButton::LEFT);
	_stereo_button.set_corner_mask (ArdourButton::RIGHT);
	_mono_button.set_active (true);
	_mono_button.signal_clicked.connect ([this] () { _mono_button.set_active (true); _stereo_button.set_active (false); });
	_stereo_button.signal_clicked.connect ([this] () { _mono_button.set_active (false); _stereo_button.set_active (true); });
	_channels_box.set_homogeneous (true);
	_channels_box.set_spacing (1);
	_channels_box.pack_start (_mono_button, true, true);
	_channels_box.pack_start (_stereo_button, true, true);

	_name_label.set_text (_("Name"));
	_count_label.set_text (_("How many"));
	_channels_label.set_text (_("Channels"));
	_instrument_label.set_text (_("Instrument"));
	Label* labels[] = { &_name_label, &_count_label, &_channels_label, &_instrument_label };
	for (int i = 0; i < 4; ++i) {
		labels[i]->set_alignment (0, 0.5);
		labels[i]->set_name ("QuickAddLabel");
	}

	_name_entry.set_activates_default (true);
	_name_entry.set_width_chars (16);
	_name_entry.signal_activate ().connect (sigc::mem_fun (*this, &QuickAddRouteWindow::add_clicked));
	_count_spin.set_numeric (true);
	_count_spin.set_activates_default (true);

	_add_button.set_name ("primary button");
	_add_button.signal_clicked.connect (sigc::mem_fun (*this, &QuickAddRouteWindow::add_clicked));
	_more_button.set_name ("generic button");
	_more_button.signal_clicked.connect (sigc::mem_fun (*this, &QuickAddRouteWindow::more_clicked));

	set_tooltip (_more_button, _("Open the full Add Track/Bus/VCA dialog (templates, groups, insert position, ...)"));
	set_tooltip (_arm_button, _("Record-enable the new track(s) right away"));

	Table* t = manage (new Table (6, 2, false));
	t->set_row_spacings (6);
	t->set_col_spacings (10);
	int row = 0;
	t->attach (*kind_box,          0, 2, row, row + 1, FILL, SHRINK); ++row;
	t->attach (_name_label,        0, 1, row, row + 1, FILL, SHRINK);
	t->attach (_count_label,       1, 2, row, row + 1, FILL, SHRINK); ++row;
	t->attach (_name_entry,        0, 1, row, row + 1, FILL|EXPAND, SHRINK);
	t->attach (_count_spin,        1, 2, row, row + 1, FILL, SHRINK); ++row;
	t->attach (_channels_label,    0, 2, row, row + 1, FILL, SHRINK); ++row;
	t->attach (_channels_box,      0, 2, row, row + 1, FILL, SHRINK); ++row;
	t->attach (_instrument_label,  0, 2, row, row + 1, FILL, SHRINK); ++row;
	t->attach (_instrument,        0, 2, row, row + 1, FILL, SHRINK); ++row;

	HBox* actions = manage (new HBox (false, 6));
	actions->pack_start (_more_button, false, false);
	actions->pack_end (_add_button, false, false);

	VBox* vbox = manage (new VBox (false, 10));
	vbox->set_border_width (14);
	vbox->pack_start (_title, false, false);
	vbox->pack_start (*t, false, false);
	vbox->pack_start (_arm_button, false, false);
	vbox->pack_start (*manage (new HSeparator), false, false);
	vbox->pack_start (*actions, false, false);

	/* a thin frame, since the window has no decorations */
	Frame* frame = manage (new Frame);
	frame->set_shadow_type (SHADOW_OUT);
	frame->add (*vbox);
	add (*frame);

	set_kind (AudioTrack);
	frame->show_all ();
}

void
QuickAddRouteWindow::set_kind (Kind k)
{
	_kind = k;

	_audio_button.set_active (k == AudioTrack);
	_midi_button.set_active (k == MidiTrack);
	_bus_button.set_active (k == AudioBus);

	switch (k) {
		case AudioTrack:
			_name_entry.set_text (_("Audio"));
			_add_button.set_text (_("Add Track"));
			break;
		case MidiTrack:
			_name_entry.set_text (_("MIDI"));
			_add_button.set_text (_("Add Track"));
			break;
		case AudioBus:
			_name_entry.set_text (_("Bus"));
			_add_button.set_text (_("Add Bus"));
			/* busses are usually stereo */
			_mono_button.set_active (false);
			_stereo_button.set_active (true);
			break;
	}

	update_sensitivity ();
}

void
QuickAddRouteWindow::update_sensitivity ()
{
	const bool audio = (_kind != MidiTrack);

	_channels_label.set_visible (audio);
	_channels_box.set_visible (audio);
	_instrument_label.set_visible (!audio);
	_instrument.set_visible (!audio);
	_arm_button.set_visible (_kind != AudioBus);
}

void
QuickAddRouteWindow::popup_at (int root_x, int root_y)
{
	set_kind (_kind);
	_count_adj.set_value (1);
	move (root_x, root_y);
	present ();
	_name_entry.grab_focus ();
	_name_entry.select_region (0, -1);
}

bool
QuickAddRouteWindow::on_key_press_event (GdkEventKey* ev)
{
	if (ev->keyval == GDK_Escape) {
		hide ();
		return true;
	}
	return ArdourWindow::on_key_press_event (ev);
}

bool
QuickAddRouteWindow::on_focus_out_event (GdkEventFocus* ev)
{
	bool rv = ArdourWindow::on_focus_out_event (ev);
	/* behave like a popover: dismiss when focus moves elsewhere, but not
	 * while our own instrument drop-down is open */
	if (!_instrument.property_popup_shown ().get_value ()) {
		hide ();
	}
	return rv;
}

void
QuickAddRouteWindow::more_clicked ()
{
	hide ();
	ARDOUR_UI::instance ()->add_route ();
}

void
QuickAddRouteWindow::add_clicked ()
{
	Session* s = ARDOUR_UI::instance ()->the_session ();
	if (!s) {
		hide ();
		return;
	}

	const uint32_t how_many = std::max (1, _count_spin.get_value_as_int ());
	const string   name     = _name_entry.get_text ();
	const bool     arm      = _arm_button.get_active () && _kind != AudioBus;
	const bool     strict   = Config->get_strict_io () || Profile->get_mixbus ();

	/* same channel logic as the full Add Route dialog */
	const int32_t in_chn = _stereo_button.get_active () ? 2 : 1;
	int32_t out_chn      = in_chn;
	if (Config->get_output_auto_connect () & AutoConnectMaster) {
		out_chn = s->master_out () ? s->master_out ()->n_inputs ().n_audio () : in_chn;
	}

	hide ();

	RouteList added;

	try {
		Session::ProcessorChangeBlocker pcb (s);

		switch (_kind) {
			case AudioTrack:
				{
					std::list<std::shared_ptr<ARDOUR::AudioTrack> > tracks = s->new_audio_track (in_chn, out_chn, std::shared_ptr<RouteGroup> (), how_many, name, PresentationInfo::max_order, ARDOUR::Normal, true, UIConfiguration::instance ().get_show_on_cue_page ());
					for (auto const& t : tracks) {
						t->set_strict_io (strict);
						added.push_back (t);
					}
				}
				break;
			case MidiTrack:
				{
					ChanCount one_midi_channel;
					one_midi_channel.set (DataType::MIDI, 1);
					std::list<std::shared_ptr<ARDOUR::MidiTrack> > tracks = s->new_midi_track (one_midi_channel, one_midi_channel, strict, _instrument.selected_instrument (), 0, std::shared_ptr<RouteGroup> (), how_many, name, PresentationInfo::max_order, ARDOUR::Normal, true, UIConfiguration::instance ().get_show_on_cue_page ());
					added.insert (added.end (), tracks.begin (), tracks.end ());
				}
				break;
			case AudioBus:
				added = s->new_audio_route (in_chn, out_chn, std::shared_ptr<RouteGroup> (), how_many, name, PresentationInfo::AudioBus, PresentationInfo::max_order);
				for (auto const& r : added) {
					r->set_strict_io (strict);
				}
				break;
		}
	} catch (...) {
		ARDOUR_UI::instance ()->display_insufficient_ports_message ();
		return;
	}

	if (added.size () != how_many) {
		error << string_compose (P_("could not create %1 new track or bus", "could not create %1 new tracks or busses", how_many), how_many) << endmsg;
	}

	if (arm) {
		for (auto const& r : added) {
			std::shared_ptr<Track> t = std::dynamic_pointer_cast<Track> (r);
			if (t) {
				t->rec_enable_control ()->set_value (1.0, Controllable::NoGroup);
			}
		}
	}
}
