ardour { ["type"] = "SessionInit", name = "UI QA smoke session" }

-- Run by tools/ui-qa/smoke.sh as a session template ("urn:ardour:<this file>").
-- Exercises UI code paths touched by the redesign; smoke.sh then checks the
-- saved session and the log. Every step prints a marker so a crash can be
-- located in the log.

function factory () return function ()
	local wav = os.getenv ("ARDOUR_QA_WAV")

	print ("QA: import")
	if wav then
		local files = C.StringVector ()
		files:push_back (wav)
		local pos = Temporal.timepos_t (0)
		Editor:do_import (files,
			Editing.ImportDistinctFiles, Editing.ImportAsTrack, ARDOUR.SrcQuality.SrcBest,
			ARDOUR.MidiTrackNameSource.SMFFileAndTrackName, ARDOUR.MidiTempoMapDisposition.SMFTempoIgnore,
			pos, ARDOUR.PluginInfo (), ARDOUR.Track (), false)
	end

	print ("QA: bus")
	Session:new_audio_route (2, 2, ARDOUR.RouteGroup (), 1, "QA Bus", ARDOUR.PresentationInfo.Flag.AudioBus, ARDOUR.PresentationInfo.max_order)

	-- every edit tool: exercises the active-tool labels
	print ("QA: mouse modes")
	for _, m in ipairs ({ Editing.MouseRange, Editing.MouseCut, Editing.MouseTimeFX,
	                      Editing.MouseGrid, Editing.MouseDraw, Editing.MouseContent,
	                      Editing.MouseObject }) do
		Editor:set_mouse_mode (m, false)
	end

	-- every page: exercises the page switcher and each page's construction
	print ("QA: pages")
	for _, a in ipairs ({ "show-mixer", "show-trigger", "show-recorder", "show-editor" }) do
		Editor:access_action ("Common", a)
	end

	Session:save_state ("", false, false, false, false, false)

	-- leave the quick-add popover open; smoke.sh types a name + Return
	print ("QA: quick-add")
	Editor:access_action ("Editor", "quick-add-track")
	print ("QA: script done")
end end
