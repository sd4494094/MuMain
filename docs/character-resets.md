# Character reset NPCs

On servers with the reset-dialog extension enabled, speaking to Leo opens the server's
live reset requirements and result. Querying does not change the character. Apply for
reset displays a review step; only **Confirm reset** performs the reset. The operation
can be confirmed once, expires after two minutes and is invalidated when the character
state changes. Leave or Escape closes the conversation.

Season 6 NPC 464 opens the stat advisor when the server also has the MuMain extension.
Choose **Stat reset** to review the current fee, level, item and equipment requirements,
the invested points to be returned and the point balance after reset. Only **Confirm
stat reset** charges and changes the character. Back, Leave and Escape do not spend
anything. The server checks the current quote again before execution.

Choose the separate **Increase Strength**, **Increase Agility**, **Increase Vitality**
or **Increase Energy** option. **Increase Leadership** appears only for classes that
can allocate it. Enter how many points to add to the selected attribute, review it,
then **Confirm allocation**. Only one attribute is editable and submitted at a time.
The server uses the same action as its existing single-attribute add commands,
without reloading the map or moving the character. The increase must be positive,
fit the normal point balance and stay within the attribute limit. Duplicate or
stale confirmations and requests containing multiple attributes cannot add points.
Master points and equipment bonuses are not part of this operation.

The new workflow requires matching MuMain client and OpenMU server versions. Older
clients keep their native stat reset dialog. The existing /resetinfo, /reset and
single-attribute add commands remain available.

Leo and Erin use fixed right-side panels with the same NPC frame, cyan name heading
and native text size as ordinary NPC dialogue. They cannot be dragged. Erin's
instructions and allocation table have separate bounded regions; entering values
does not spend points until the review is explicitly confirmed. X and Escape leave
the conversation, while Back stays within it.
