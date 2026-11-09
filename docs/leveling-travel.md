# Leveling travel NPC

On servers with this optional service enabled, speak to **Travel Guide Noah** in
Lorencia at **143, 131**. The server supplies a scrollable list with the full map/floor
name, location description, your effective required level and the Zen price.

The fixed right-side panel uses the ordinary NPC dialogue frame, title and native
text size. Rows show the full map/floor and location description on one line. A fixed
summary above the list shows the initial destination and updates on hover or focus,
including the effective level requirement and Zen price even for unavailable rows.
The name, required level and price occupy separate lines; long names wrap within
the summary. Destinations are sorted by the effective required level, lowest first.
Equal-level destinations retain the server order, and their travel identifiers do
not change when reordered. Only destinations scroll, and the
native X button stays visible. The panel cannot be dragged. The shared UI scale still applies. “五点练级区” retains the historical location
name; it does not describe a compass direction or a time.

Click an enabled row to travel immediately. Close or Escape cancels without spending
anything and ends the NPC conversation. Locations are shared map positions, not private
rooms. This service leaves the ordinary **M** travel window unchanged.

Both the server and client must support the leveling-travel extension. The server
controls availability, conditions, fees and coordinates. Bot accounts cannot use this
NPC. If a destination or price changes while the list is open, reopen the conversation
to obtain the current choices.

## Town NPC presentation

The matching client data gives Erin (464) her own female robe model, while Leo (371)
keeps his excellent Black Dragon equipment and Storm wings in the flying idle pose,
including in town safe zones. These are cosmetic changes; NPC services keep their
server-side rules.

The Lorencia Tab map resource includes Noah, Leo, Erin at the tavern door (130,133),
and all four configured Phantom Helper locations. When server placements change,
update the localized minimap data as well. The private client package preserves all
portals and other markers and compacts empty records so later markers remain visible.

Operators can maintain a licensed minimap with `tools/update_minimap_npcs.py`,
passing separate `--input`, `--placements` and new `--output` paths. The placement JSON
lists names and all their tile coordinates. The tool checks the checksum, UTF-8
field lengths, duplicate placements and file capacity. Back up the installed file
and verify the markers against server spawns before publishing.

The female stat-reset appearance belongs to server NPC number 464. Number 414 is a
different helper and retains its original appearance. Client updates must be installed
with the game closed; an archived update on the server does not update a running client.
NPC speech bubbles show the current utterance without stacking repeated idle lines;
player chat retains its previous-line behavior.

## Creator Goddess Aria

The matching private client adds **创世神·艾莉雅 (Aria)**, a female decorative NPC
at **Lorencia 147, 133**, east of Noah. Her independent wire number is **711** and
client model slot is **220**. Her model combines adapted native MU geometry, animated
wings, a halo and raised flying poses; the concept artwork is a visual direction,
not a screenshot of the finished low-poly model. The upgraded appearance uses
independent animated Illusion-style wings in a platinum, gold and violet palette,
with +15/excellent equipment glow. Her double halo has pulsing light and orbiting
star points. Equipment glow follows the client's effect quality settings; compare
the effect with nearby upgraded equipment using the same settings.

Existing Season 6 installations use the optional **Add creator goddess NPC** update.
Fresh configuration includes her by default. The **Creator goddess NPC** talk plugin
can be disabled independently. Initialization preserves an existing NPC's naming,
placements and plugin activation. This update does not add gameplay rewards or
teleport destinations. Clicking her shows a localized greeting and immediately
releases the NPC conversation, so another NPC can be opened afterwards.

Install the matching client executable, monster definitions, private Aria model and
textures, and Lorencia minimap **before enabling her server spawn**. Older clients
have no binding for number 711. Keep private model assets outside the public Git
repository and preserve licensed source assets. Back up changed files with the game
closed; verify hover motion, name, Tab marker, selection bounds, greeting and nearby
NPC access in the actual client after publication.

The wing/halo appearance upgrade is client-only: install its matching executable,
monster model definitions and private Aria assets together, with the game closed.
It needs no server restart and changes neither player equipment nor NPC placement.
Preserve the previous client backup until the appearance is verified in game.
