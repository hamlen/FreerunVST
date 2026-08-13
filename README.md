# FreerunVST

*FreerunVST* is a VST3 metaplugin that augments any other VST3 plugin with the ability to desync from the host's transport and enter "freerunning" mode on demand.  While in freerunning mode, the affected plugin continues at the tempo that was playing at the moment it started freerunning, ignoring any changes to the host's tempo until freerunning mode is deactivated.  When freerunning mode is deactivated, FreerunVST jumps the plugin's transport to the host's current position, tempo, and time signature, and remains synced with the host transport until freerunning mode is reactivated.

This per-plugin freerunning capability facilitates musical transitions in which an instrument fades out at one tempo while a new instrument begins playing at a new tempo. To achieve this, activate the first instrument's freerunning mode before the fade-out begins, switch to the new tempo, and begin playing the second instrument. Since the first instrument is freerunning at the old tempo, it ignores the tempo change even while the second instrument plays at the new tempo. In general, locking plugins to different tempos in this way facilitates polymeter performances and compositions without freezing tracks.

To minimize overhead and maximize compatibility, *FreerunVST* implements this new capability as a thin overlay of the hosted plugin which looks and acts exactly like the hosted plugin. The only visible difference introduced by the overlay is that alongside the hosted plugin's usual automation parameters, a Freerun-enhanced plugin exports one extra parameter named **Freerun** that turns the affected plugin's new freerunning mode on (1.0) or off (0.0).

*FreerunVST* can also permanently lock a plugin to a pre-defined tempo if desired. To do so, temporarily set the host to the desired tempo, switch the plugin to freerunning mode, and leave it in freerunning mode forever. When your host saves the plugin state, *FreerunVST* saves the frerunning tempo and time signature as part of the instance's internal state, and will continue to use that saved state when that instance is reloaded into future sessions.

## Installation:

To install *FreerunVST*, copy the "FreerunVST64.vst3" file into the same folder as the "xxxx.vst3" file of the plugin you want to freerun, and rename "FreerunVST64.vst3" to "Freerun-xxxx.vst3", where "xxxx.vst3" is the exact file name of the plugin you want to freerun.  For example, if you have a plugin named "Cool Instrument.vst3" in your VST3 folder, rename "FreerunVST64.vst3" to "Freerun-Cool Instrument.vst3" and put it in the same folder as "Cool Instrument.vst3".  (Or make a shortcut named "xxxx.vst3" to your plugin, and put the shortcut in the same folder as "Freerun-xxxx.vst3".)

When your host next scans for plugins, *FreerunVST* will look at its own filename and become a plugin named "PluginName (FR)" where "PluginName" is the name reported by your "xxxx.vst3" plugin. Loading "PluginName (FR)" from your host's plugin list will load a freerunnable instance of the "PluginName" plugin. To freerun multiple plugins, just make multiple copies of "FreerunVST64.vst3", each named "Freerun-xxxx.vst3", "Freerun-yyyy.vst3", etc.  Remember that each copy must be located in the same folder (respectively) as "xxxx.vst3", "yyyy.vst3", etc.

### Change History

* v1.0 - initial release
* v1.1 - support loading plugins via shortcuts