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

        void InitializeData(RE::BGSListForm* a_breathShoutList, 
                            RE::BGSListForm* a_ballShoutList,
                            RE::TESShout* a_unrelentingForceShout,
                            RE::TESShout* a_attackShout,
                            RE::BGSRefAlias* a_combatTargetAlias);

        void SetStopCombat(bool a_stop, bool a_calledFromPapyrus = false);

        void SetAttackDisabled(bool a_disabled);

        bool GetAttackDisabled();
 
        bool DragonAttack(bool a_alternateAttack = false);
        
        RE::TESObjectREFR* GetCombatTarget();

        bool SyncCombatTarget();

        RE::BGSListForm* GetBreathShoutList();

        void SetBreathShoutList(RE::BGSListForm* a_breathShoutList);

        RE::BGSListForm* GetBallShoutList();

        void SetBallShoutList(RE::BGSListForm* a_ballShoutList);

    private:
        CombatManager() = default;
        ~CombatManager() = default;

        // accessed by other classes
        // change value only via Set function to trigger PropertyUpdateEvent
        RE::BGSRefAlias* m_combatTargetAlias = nullptr;
        RE::BGSListForm* m_breathShoutList = nullptr;
        RE::BGSListForm* m_ballShoutList = nullptr;
        RE::TESShout* m_unrelentingForceShout = nullptr;
        RE::TESShout* m_attackShout = nullptr;
        bool m_stopCombat = false; // this flag is used to stop combat whenever a FastTravel ends
        const float m_maxTargetDistance = 2000.0f;

        // only used internally
        bool m_registeredForTargetSync = false;
        bool m_attackDisabled = false;

        RE::TESShout* GetShout(const RE::BGSListForm* a_shoutList);

        bool SetShoutMode(int a_shoutMode);

        void DragonStartCombat(RE::Actor* a_target);

        float GetMaxTargetDistance();
    
        bool GetStopCombat();
    }; // class CombatManager
} // namespace IDRC

