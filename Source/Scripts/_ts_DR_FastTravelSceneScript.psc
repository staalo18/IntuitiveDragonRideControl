;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
;NEXT FRAGMENT INDEX 16
Scriptname _ts_DR_FastTravelSceneScript Extends Scene Hidden

import _ts_DR_Debug

bool bFromPatrolQueued = false

;BEGIN FRAGMENT Fragment_0
Function Fragment_0()
;BEGIN CODE
; FTScene begin
_ts_Debug_Trace_Message("IDRC - _ts_DR_FastTravelSceneScript: starting FastTravel Scene")
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_6
Function Fragment_6()
;BEGIN CODE
; start Fast Travelling
_ts_Debug_Trace_Message("IDRC - _ts_DR_FastTravelSceneScript: start Fast Travelling (no Patrol queued)")
    
    if !bFromPatrolQueued ; only if not directly coming from patrolqueued
        ; See comment in _ts_DR_RideControlScript->CancelStopFastTravel():
        ( GetOwningQuest() as _ts_DR_RideControlScript ).CancelStopFastTravel()
    endif

    bFromPatrolQueued = false
    actor dragon = ( GetOwningQuest() as _ts_DR_RideControlScript ).dragonAlias.GetActorRef()
    
    if !dragon.IsAllowedToFly()
        ( GetOwningQuest() as _ts_DR_RideControlScript ).StopFastTravelAsEvent(dragon)
_ts_Debug_Trace_Message("IDRC - _ts_DR_FastTravelSceneScript: started FastTravel while dragon not allowed to fly!")  
    endif
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_8
Function Fragment_8()
;BEGIN CODE
; end Fast travelling
 ;   ( GetOwningQuest() as _ts_DR_RideControlScript ).SetAttackDisabled(true)
    ObjectReference Marker = ( GetOwningQuest() as _ts_DR_RideControlScript ).DLC2TameDragonOrbitMarker
    actor dragon = ( GetOwningQuest() as _ts_DR_RideControlScript ).dragonAlias.GetActorRef()

    float fdiffx = dragon.GetPositionX() - Marker.GetPositionX()
    float fdiffy = dragon.GetPositionY() - Marker.GetPositionY()
    float fDistance = math.sqrt(fdiffx*fdiffx + fdiffy*fdiffy)
 _ts_Debug_Trace_Message("IDRC - _ts_DR_FastTravelSceneScript: end Fast travelling: distance = " + fDistance) 

    if !dragon.IsAllowedToFly()
_ts_Debug_Trace_Message("IDRC - _ts_DR_FastTravelSceneScript: FastTravel ended while dragon not allowed to fly - calling TriggerLand")  
        ( GetOwningQuest() as _ts_DR_RideControlScript ).TriggerLand()
    endif
     
    bFromPatrolQueued = true
    RegisterForSingleUpdate(0.2) ; resets bFromPatrolQueued flag in 0.2s

;debug.MessageBox("end Fast travelling: distance = " +fDistance) 
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_4
Function Fragment_4()
;BEGIN CODE
; not fast travelling - end
_ts_Debug_Trace_Message("IDRC - _ts_DR_FastTravelSceneScript: not fast travelling - end")
;debug.MessageBox("not fast travelling - end")
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_1
Function Fragment_1()
;BEGIN CODE
; FT Scene end
_ts_Debug_Trace_Message("IDRC - _ts_DR_FastTravelSceneScript: Error - FastTravel Scene stopped!")
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_12
Function Fragment_12()
;BEGIN CODE
; start FastTravel AND Patrol queued
_ts_Debug_Trace_Message("IDRC - _ts_DR_FastTravelSceneScript: Error - start FastTravel AND Patrol queued")
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_14
Function Fragment_14()
;BEGIN CODE
; end Fast Travel AND patrol queued
_ts_Debug_Trace_Message("IDRC - _ts_DR_FastTravelSceneScript: Error - end Fast Travel AND patrol queued")
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_11
Function Fragment_11()
;BEGIN CODE
; end patrol queued
_ts_Debug_Trace_Message("IDRC - _ts_DR_FastTravelSceneScript: Patrol queued ended")

    bFromPatrolQueued = true
    RegisterForSingleUpdate(0.2) ; resets bFromPatrolQueued flag in 0.2s
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_2
Function Fragment_2()
;BEGIN CODE
; not FastTraveling - start
_ts_Debug_Trace_Message("IDRC - _ts_DR_FastTravelSceneScript: not FastTraveling - start")
    bFromPatrolQueued = false ; reset patrol queued flag

;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_9
Function Fragment_9()
;BEGIN CODE
; start patrol queued
_ts_Debug_Trace_Message("IDRC - _ts_DR_FastTravelSceneScript: start patrol queued (no FastTravel)")
    ; See comment in _ts_DR_RideControlScript->CancelStopFastTravel():
    ( GetOwningQuest() as _ts_DR_RideControlScript ).CancelStopFastTravel()
 
    actor dragon = ( GetOwningQuest() as _ts_DR_RideControlScript ).dragonAlias.GetActorRef()
    if !dragon.IsAllowedToFly()
        ( GetOwningQuest() as _ts_DR_RideControlScript ).StopFastTravelAsEvent(dragon)
_ts_Debug_Trace_Message("IDRC - _ts_DR_FastTravelSceneScript: started FastTravel while dragon not allowed to fly!")  
    endif
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment

Event OnUpdate()
    ; reset flag
    bFromPatrolQueued = false
endEvent
