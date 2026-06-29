;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
;NEXT FRAGMENT INDEX 7
Scriptname _ts_DR_BorderRegionSceneScript Extends Scene Hidden

import _ts_DR_Debug

bool bIsPlayerInBorderRegion = false

;BEGIN FRAGMENT Fragment_3
Function Fragment_3()
;BEGIN CODE
; begin scene
_ts_Debug_Trace_Message("IDRC - _ts_DR_BorderRegionSceneScript: starting BorderRegion Scene")
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_4
Function Fragment_4()
;BEGIN CODE
; end scene
_ts_Debug_Trace_Message("IDRC - _ts_DR_BorderRegionSceneScript: Error - BorderRegion Scene stopped!")
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_2
Function Fragment_2()
;BEGIN CODE
; start phase 2 (not in border region)
_ts_Debug_Trace_Message("IDRC - _ts_DR_BorderRegionSceneScript: Player left BorderRegion")
bIsPlayerInBorderRegion = false
actor dragonActor = (GetOwningQuest() as _ts_DR_RideControlScript).dragonAlias.GetActorRef()
if dragonActor && dragonActor.IsBeingRidden()
    debug.Notification("You cannot go that way :-(")
    ;(GetOwningQuest() as _ts_DR_RideControlScript).DragonUnregisterForControls()
    (GetOwningQuest() as _ts_DR_RideControlScript).ForceHover()
endif
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_0
Function Fragment_0()
;BEGIN CODE
; start phase 1 (in border region)
_ts_Debug_Trace_Message("IDRC - _ts_DR_BorderRegionSceneScript: Player entered BorderRegion")
bIsPlayerInBorderRegion = true
;(GetOwningQuest() as _ts_DR_RideControlScript).DragonRegisterForControls()

;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment

bool function IsPlayerInBorderRegion()
    return bIsPlayerInBorderRegion
endFunction
