#pragma once


namespace IDRC {   
    class FlyingModeManager;

    class CombatManager {
    public:
        static CombatManager& GetSingleton() {
            static CombatManager instance;
            return instance;
        }
        CombatManager(const CombatManager&) = delete;
        CombatManager& operator=(const CombatManager&) = delete;

        void InitializeData(RE::BGSListForm* a_targetPackageList, 
                            RE::BGSListForm* a_breathShoutList, 
                            RE::BGSListForm* a_ballShoutList,
                            RE::TESShout* a_unrelentingForceShout,
                            RE::TESShout* a_attackShout,
                            RE::BGSRefAlias* a_combatTargetAlias);

        int GetAttackMode();
    
        void SetWaitForShout(bool a_wait, bool a_calledFromPapyrus = false);

        void SetStopCombat(bool a_stop, bool a_calledFromPapyrus = false);

        void SetAttackDisabled(bool a_disabled);

        bool GetAttackDisabled();

        bool GetTriggerAttack();

        void SetTriggerAttack(bool a_trigger);

        bool StopAttack(bool a_forceStop = false);
 
        bool DragonAttack(bool a_alternateAttack = false);
        
        RE::TESObjectREFR* GetCombatTarget();

        bool IsAttackOngoing();

        bool SyncCombatTarget(bool a_forceSync = false);

        RE::BGSListForm* GetCombatTargetPackageList();

        RE::BGSListForm* GetBreathShoutList();

        void SetBreathShoutList(RE::BGSListForm* a_breathShoutList);

        RE::BGSListForm* GetBallShoutList();

        void SetBallShoutList(RE::BGSListForm* a_ballShoutList);

        bool IsAutoCombatAttackToggled();

    private:
        CombatManager() = default;
        ~CombatManager() = default;

        // accessed by other classes
        // change value only via Set function to trigger PropertyUpdateEvent
        RE::BGSRefAlias* m_combatTargetAlias = nullptr;
        RE::BGSListForm* m_combatTargetPackageList = nullptr;
        RE::BGSListForm* m_breathShoutList = nullptr;
        RE::BGSListForm* m_ballShoutList = nullptr;
        RE::TESShout* m_unrelentingForceShout = nullptr;
        RE::TESShout* m_attackShout = nullptr;
        int m_attackMode = 0;
        bool m_waitForShout = false;
        bool m_stopCombat = false; // this flag is used to stop combat whenever a FastTravel ends
        const float m_maxTargetDistance = 2000.0f;

        // only used internally
        bool m_toggledAutoCombatAttack = false;
        bool m_registeredForAttack = false;
        bool m_registeredForTargetSync = false;
        bool m_attackDisabled = false;
        bool m_triggerAttack = false;

        RE::TESShout* GetShout(const RE::BGSListForm* a_shoutList);

        bool SetShoutMode(int a_shoutMode);

        void DragonStartCombat(RE::Actor* a_target);

        void SetAttackMode(int a_mode);

        bool GetWaitForShout();

        float GetMaxTargetDistance();
    
        bool GetStopCombat();
    }; // class CombatManager
} // namespace IDRC

