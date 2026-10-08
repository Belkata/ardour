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

#include "pbd/compose.h"
#include "pbd/error.h"

#include "ardour/audio_track.h"
#include "ardour/audioengine.h"
#include "ardour/io.h"
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
	, _input (InputAuto)
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
	_mono_button.signal_clicked.connect (sigc::bind (sigc::mem_fun (*this, &QuickAddRouteWindow::set_channels), false));
	_stereo_button.signal_clicked.connect (sigc::bind (sigc::mem_fun (*this, &QuickAddRouteWindow::set_channels), true));
	_channels_box.set_homogeneous (true);
	_channels_box.set_spacing (1);
	_channels_box.pack_start (_mono_button, true, true);
	_channels_box.pack_start (_stereo_button, true, true);

	_name_label.set_text (_("Name"));
	_count_label.set_text (_("How many"));
	_channels_label.set_text (_("Channels"));
	_instrument_label.set_text (_("Instrument"));
	_input_label.set_text (_("Record from"));
	Label* labels[] = { &_name_label, &_count_label, &_channels_label, &_instrument_label, &_input_label };
	for (int i = 0; i < 5; ++i) {
		labels[i]->set_alignment (0, 0.5);
		labels[i]->set_name ("QuickAddLabel");
	}

	_name_entry.set_width_chars (16);
	_name_entry.signal_activate ().connect (sigc::mem_fun (*this, &QuickAddRouteWindow::add_clicked));
	_count_spin.set_numeric (true);
	_count_spin.signal_activate ().connect (sigc::mem_fun (*this, &QuickAddRouteWindow::add_clicked));

	_add_button.set_name ("primary button");
	_add_button.signal_clicked.connect (sigc::mem_fun (*this, &QuickAddRouteWindow::add_clicked));
	_more_button.set_name ("generic button");
	_more_button.signal_clicked.connect (sigc::mem_fun (*this, &QuickAddRouteWindow::more_clicked));

	set_tooltip (_more_button, _("Open the full Add Track/Bus/VCA dialog (templates, groups, insert position, ...)"));
	set_tooltip (_arm_button, _("Record-enable the new track(s) right away"));
	set_tooltip (_input_selector, _("Which hardware input the new track records from.\n"
	                                "<b>Automatic</b> follows the input auto-connect preference.\n"
	                                "When adding several audio tracks, each track takes the next input(s)."));

	_input_selector.set_name ("generic button");

	Table* t = manage (new Table (8, 2, false));
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
	t->attach (_input_label,       0, 2, row, row + 1, FILL, SHRINK); ++row;
	t->attach (_input_selector,    0, 2, row, row + 1, FILL, SHRINK); ++row;

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

	/* show everything first; set_kind() then hides what does not apply */
	frame->show_all ();
	set_kind (AudioTrack);
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

	refill_inputs ();
	update_sensitivity ();
}

void
QuickAddRouteWindow::set_channels (bool stereo)
{
	_mono_button.set_active (!stereo);
	_stereo_button.set_active (stereo);
	refill_inputs ();
}

std::string
QuickAddRouteWindow::input_label (int i) const
{
	if (i < 0 || i >= (int) _phys_inputs.size ()) {
		return "";
	}
	string const& port = _phys_inputs[i];
	string        pn   = AudioEngine::instance ()->get_pretty_name_by_name (port);
	if (pn.empty ()) {
		/* e.g. "system:capture_1" -> "capture_1" */
		string::size_type colon = port.find (':');
		pn = (colon != string::npos) ? port.substr (colon + 1) : port;
	}
	return pn;
}

void
QuickAddRouteWindow::refill_inputs ()
{
	_phys_inputs.clear ();
	if (_kind == MidiTrack) {
		AudioEngine::instance ()->get_physical_inputs (DataType::MIDI, _phys_inputs, MidiPortFlags (0), MidiPortFlags (MidiPortControl | MidiPortVirtual));
	} else {
		AudioEngine::instance ()->get_physical_inputs (DataType::AUDIO, _phys_inputs);
	}

	using namespace Menu_Helpers;
	_input_selector.clear_items ();
	_input_selector.add_menu_elem (MenuElem (_("Automatic"), sigc::bind (sigc::mem_fun (*this, &QuickAddRouteWindow::set_input), (int) InputAuto)));
	_input_selector.add_menu_elem (MenuElem (_("No input"), sigc::bind (sigc::mem_fun (*this, &QuickAddRouteWindow::set_input), (int) InputNone)));

	const bool stereo = _kind != MidiTrack && _stereo_button.get_active ();
	const int  n      = _phys_inputs.size ();
	if (n > 0) {
		_input_selector.add_separator ();
	}
	/* stereo: pairs of adjacent inputs (1+2, 3+4, ...) */
	for (int i = 0; i + (stereo ? 1 : 0) < n; i += (stereo ? 2 : 1)) {
		string label = input_label (i);
		if (stereo) {
			label = string_compose (_("%1 + %2"), label, input_label (i + 1));
		}
		_input_selector.add_menu_elem (MenuElem (label, sigc::bind (sigc::mem_fun (*this, &QuickAddRouteWindow::set_input), i)));
	}

	/* keep the current choice if it still exists */
	int in = _input;
	if (in >= 0 && (in + (stereo ? 1 : 0) >= n || (stereo && (in % 2)))) {
		in = InputAuto;
	}
	set_input (in);
}

void
QuickAddRouteWindow::set_input (int i)
{
	_input = i;
	switch (i) {
		case InputAuto:
			_input_selector.set_text (_("Automatic"));
			break;
		case InputNone:
			_input_selector.set_text (_("No input"));
			break;
		default:
			if (_kind != MidiTrack && _stereo_button.get_active ()) {
				_input_selector.set_text (string_compose (_("%1 + %2"), input_label (i), input_label (i + 1)));
			} else {
				_input_selector.set_text (input_label (i));
			}
			break;
	}
}

void
QuickAddRouteWindow::connect_inputs (RouteList const& routes)
{
	if (_input < 0) {
		return;
	}

	const DataType dt  = (_kind == MidiTrack) ? DataType::MIDI : DataType::AUDIO;
	size_t         idx = _input;

	for (auto const& r : routes) {
		std::shared_ptr<IO> io = r->input ();
		/* MIDI: every track listens to the chosen device;
		 * audio: each track takes the next input(s) */
		if (dt == DataType::MIDI) {
			idx = _input;
		}
		for (uint32_t c = 0; c < io->n_ports ().get (dt); ++c, ++idx) {
			if (idx >= _phys_inputs.size ()) {
				return;
			}
			io->connect (io->ports ()->port (dt, c), _phys_inputs[idx]);
		}
	}
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
	_input_label.set_visible (_kind != AudioBus);
	_input_selector.set_visible (_kind != AudioBus);
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
	 * while one of our own drop-downs is open */
	if (!_instrument.property_popup_shown ().get_value () && !_input_selector.menu ().get_visible ()) {
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
	/* auto-connect inputs only when no explicit input was chosen */
	const bool     auto_in  = (_input == InputAuto);

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
					std::list<std::shared_ptr<ARDOUR::AudioTrack> > tracks = s->new_audio_track (in_chn, out_chn, std::shared_ptr<RouteGroup> (), how_many, name, PresentationInfo::max_order, ARDOUR::Normal, auto_in, UIConfiguration::instance ().get_show_on_cue_page ());
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
					std::list<std::shared_ptr<ARDOUR::MidiTrack> > tracks = s->new_midi_track (one_midi_channel, one_midi_channel, strict, _instrument.selected_instrument (), 0, std::shared_ptr<RouteGroup> (), how_many, name, PresentationInfo::max_order, ARDOUR::Normal, auto_in, UIConfiguration::instance ().get_show_on_cue_page ());
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

	if (_kind != AudioBus) {
		connect_inputs (added);
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
