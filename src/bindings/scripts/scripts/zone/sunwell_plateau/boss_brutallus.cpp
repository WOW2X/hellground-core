/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* ScriptData
SDName: Boss_Brutallus
SD%Complete: 98
SDComment: Check Felmyst spawning event, final debugging
EndScriptData */

#include "precompiled.h"
#include "def_sunwell_plateau.h"

enum Quotes
{
 YELL_INTRO                 =   -1580017,
 YELL_INTRO_BREAK_ICE       =   -1580018,
 YELL_INTRO_CHARGE          =   -1580019,
 YELL_INTRO_KILL_MADRIGOSA  =   -1580020,
 YELL_INTRO_TAUNT           =   -1580021,

 YELL_MADR_ICE_BARRIER      =   -1580031,
 YELL_MADR_INTRO            =   -1580032,
 YELL_MADR_ICE_BLOCK        =   -1580033,
 YELL_MADR_TRAP             =   -1580034,
 YELL_MADR_DEATH            =   -1580035,

 YELL_AGGRO                 =   -1580022,
 YELL_KILL1                 =   -1580023,
 YELL_KILL2                 =   -1580024,
 YELL_KILL3                 =   -1580025,
 YELL_LOVE1                 =   -1580026,
 YELL_LOVE2                 =   -1580027,
 YELL_LOVE3                 =   -1580028,
 YELL_BERSERK               =   -1580029,
 YELL_DEATH                 =   -1580030
};

enum Spells
{
    SPELL_METEOR_SLASH                 =   45150,
    SPELL_BURN                         =   45141,
    SPELL_STOMP                        =   45185,
    SPELL_BERSERK                      =   26662,
    SPELL_DUAL_WIELD                   =   42459,
    SPELL_SUMMON_DEATH_CLOUD           =   45884,
    SPELL_DEATH_CLOUD                  =   45212,

    SPELL_INTRO_CHARGE                 =   44884,
    SPELL_INTRO_FROST_BREATH           =   45065,
    SPELL_INTRO_FROST_BLAST            =   45203,
    SPELL_INTRO_FROSTBOLT              =   44843,
    SPELL_INTRO_ENCAPSULATE            =   45661,
    SPELL_INTRO_BREAK_ICE              =   46637,
    SPELL_INTRO_BREAK_ICE_KNOCKBACK    =   47030
};

#define MADRI_FLY_X        1476.3
#define MADRI_FLY_Y           649
#define MADRI_FLY_Z          21.5

#define MOB_DEATH_CLOUD 25703
#define FELMYST 25038

struct TRINITY_DLL_DECL boss_brutallusAI : public ScriptedAI
{
    boss_brutallusAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = c->GetInstanceData();
    }

    ScriptedInstance* pInstance;

    uint32 SlashTimer;
    uint32 BurnTimer;
    uint32 StompTimer;
    uint32 BerserkTimer;
    uint32 CheckTimer;
    uint32 CheckGroundTimer;

    uint32 IntroPhase;
    uint32 IntroPhaseTimer;
    uint32 IntroFrostBoltTimer;
    bool Enraged;

    void Reset()
    {
        SlashTimer = 11000;
        StompTimer = 30000;
        BurnTimer = 20000;
        BerserkTimer = 360000;
        CheckTimer = 1000;
        CheckGroundTimer = 500;

        IntroPhase = 0;
        IntroPhaseTimer = 0;
        IntroFrostBoltTimer = 0;
        Enraged = false;

        ForceSpellCast(me, SPELL_DUAL_WIELD, INTERRUPT_AND_CAST_INSTANTLY);
        pInstance->SetData(DATA_BRUTALLUS_EVENT, NOT_STARTED);
    }

    void EnterCombat(Unit* /*pWho*/)
    {
        if (pInstance && pInstance->GetData(DATA_BRUTALLUS_INTRO_EVENT) == DONE)
        {
            DoScriptText(YELL_AGGRO, me);
            pInstance->SetData(DATA_BRUTALLUS_EVENT, IN_PROGRESS);
        }
    }

    void KilledUnit(Unit* /*victim*/)
    {
        if (pInstance && pInstance->GetData(DATA_BRUTALLUS_INTRO_EVENT) == DONE)
            DoScriptText(RAND(YELL_KILL1, YELL_KILL2, YELL_KILL3), me);
    }

    void JustDied(Unit* /*pKiller*/)
    {
        ForceSpellCastWithScriptText(me, SPELL_SUMMON_DEATH_CLOUD, YELL_DEATH, INTERRUPT_AND_CAST_INSTANTLY);
        pInstance->SetData(DATA_BRUTALLUS_EVENT, DONE);
    }

    void DamageMade(Unit* target, uint32 &damage, bool /*direct_damage*/)
    {
        if(target->GetTypeId() == TYPEID_UNIT && target->GetEntry() == 24895)
            damage *= 40;
    }

    void DoIntro()
    {
        Unit *pMadrigosa = me->GetUnit(pInstance->GetData64(DATA_MADRIGOSA));
        if (!pMadrigosa)
            return;

        switch (IntroPhase)
        {
            case 0:
                DoScriptText(YELL_MADR_ICE_BARRIER, pMadrigosa);
                pMadrigosa->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                for(uint8 i = 0; i < 8;++i)
                    pMadrigosa->SetSpeed(UnitMoveType(i), 2.5);
                pMadrigosa->GetMotionMaster()->MovePoint(1, MADRI_FLY_X, MADRI_FLY_Y, MADRI_FLY_Z);
                IntroPhaseTimer = 6500;
                ++IntroPhase;
                break;
            case 1:
                if(pInstance)
                    pInstance->SetData(DATA_BRUTALLUS_INTRO_EVENT, IN_PROGRESS);
                pMadrigosa->RemoveUnitMovementFlag(MOVEFLAG_LEVITATING | MOVEFLAG_ONTRANSPORT);
                pMadrigosa->AddUnitMovementFlag(SPLINEFLAG_WALKMODE_MODE);
                pMadrigosa->HandleEmoteCommand(EMOTE_ONESHOT_LAND);
                IntroPhaseTimer = 2500;
                ++IntroPhase;
                break;
            case 2:
                pMadrigosa->SendMovementFlagUpdate();
                DoScriptText(YELL_MADR_INTRO, pMadrigosa);
                IntroPhaseTimer = 5000;
                ++IntroPhase;
                break;
            case 3:
                float x, y, z;
                pMadrigosa->GetMap()->CreatureRelocation((Creature*)pMadrigosa, MADRI_FLY_X, MADRI_FLY_Y, MADRI_FLY_Z, me->GetOrientation());
                me->SetInFront(pMadrigosa);
                pMadrigosa->SetInFront(me);
                DoScriptText(YELL_INTRO, me);
                IntroPhaseTimer = 6000;
                ++IntroPhase;
                break;
            case 4:
                pMadrigosa->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                DoStartMovement(pMadrigosa);
                pMadrigosa->GetMotionMaster()->MoveChase(me);
                me->Attack(pMadrigosa, true);
                pMadrigosa->Attack(me, true);
                IntroPhaseTimer = 7000;
                ++IntroPhase;
                break;
            case 5:
                pMadrigosa->CastSpell(me, SPELL_INTRO_FROST_BREATH, false);
                me->CastSpell(me, SPELL_INTRO_FROST_BREATH, true);
                IntroPhaseTimer = 2500;
                ++IntroPhase;
                break;
            case 6:
                me->GetMotionMaster()->MoveIdle();
                pMadrigosa->AddUnitMovementFlag(MOVEFLAG_LEVITATING | MOVEFLAG_ONTRANSPORT);
                pMadrigosa->GetPosition(x, y, z);
                pMadrigosa->GetMotionMaster()->MovePoint(2, x, y, z+15);
                pMadrigosa->setHover(true);
                IntroPhaseTimer = 4500;
                ++IntroPhase;
            case 7:
                pMadrigosa->SetInFront(me);
                pMadrigosa->CastSpell(me, SPELL_INTRO_FROST_BLAST, false);
                me->CastSpell(me, SPELL_INTRO_FROST_BLAST, true);
                DoScriptText(YELL_MADR_ICE_BLOCK, pMadrigosa);
                IntroFrostBoltTimer = 500;
                IntroPhaseTimer = 10000;
                ++IntroPhase;
                break;
            case 8:
                DoScriptText(YELL_INTRO_BREAK_ICE, me);
                IntroPhaseTimer = 2000;
                ++IntroPhase;
                break;
            case 9:
                me->GetMotionMaster()->MoveIdle();
                me->AttackStop();
                pMadrigosa->setHover(false);
                pMadrigosa->RemoveUnitMovementFlag(MOVEFLAG_LEVITATING | MOVEFLAG_ONTRANSPORT);
                pMadrigosa->SetUnitMovementFlags(SPLINEFLAG_WALKMODE_MODE);
                pMadrigosa->HandleEmoteCommand(EMOTE_ONESHOT_LAND);
                pMadrigosa->SendMovementFlagUpdate();
                IntroPhaseTimer = 1000;
                ++IntroPhase;
                break;
            case 10:
                pMadrigosa->GetMotionMaster()->MoveIdle();
                IntroPhaseTimer = 2500;
                ++IntroPhase;
                break;
            case 11:
                pMadrigosa->GetMap()->CreatureRelocation((Creature*)pMadrigosa, MADRI_FLY_X, MADRI_FLY_Y, MADRI_FLY_Z, me->GetOrientation());
                pMadrigosa->GetMotionMaster()->MoveIdle();
                IntroPhaseTimer = 2000;
                ++IntroPhase;
                break;
            case 12:
                me->GetMotionMaster()->MoveIdle();
                pMadrigosa->CastSpell(me, SPELL_INTRO_ENCAPSULATE, false);
                DoScriptText(YELL_MADR_TRAP, pMadrigosa);
                IntroPhaseTimer = 1000;
                ++IntroPhase;
                break;
            case 13:
                me->GetPosition(x, y, z);
                me->GetMotionMaster()->MovePoint(1, x-6, y-15, z+10);
                me->SetInFront(pMadrigosa);
                IntroPhaseTimer = 8000;
                ++IntroPhase;
                break;
            case 14:
                me->RemoveAurasDueToSpell(44883);
                pMadrigosa->InterruptNonMeleeSpells(false);
                pMadrigosa->GetMotionMaster()->MoveIdle();
                DoScriptText(YELL_INTRO_CHARGE, me);
                me->SetInFront(pMadrigosa);
                me->GetPosition(x, y, z);
                float ground_Z;
                ground_Z= me->GetMap()->GetHeight(x, y, MAX_HEIGHT, true);
                me->GetMotionMaster()->MoveFall(ground_Z);
                IntroPhaseTimer = 3500;
                ++IntroPhase;
                break;  
            case 15:
                me->SetSpeed(MOVE_WALK, 10.0f, true);
                me->GetMotionMaster()->MoveCharge(MADRI_FLY_X-5, MADRI_FLY_Y-15, MADRI_FLY_Z);
                AddSpellToCast((Unit*)NULL, SPELL_INTRO_CHARGE);
                DoScriptText(YELL_MADR_DEATH, pMadrigosa);
                me->DealDamage(pMadrigosa, pMadrigosa->GetHealth(), DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, true);
                me->SetHealth(me->GetMaxHealth());
                me->CombatStop();
                IntroPhaseTimer = 4000;
                ++IntroPhase;
                break;
            case 16:
                if(pInstance)
                    me->SetFacingToObject(GameObject::GetGameObject(*me, pInstance->GetData64(DATA_BRUTALLUS_TRIGGER)));
                IntroPhaseTimer = 6000;
                ++IntroPhase;
                break;
            case 17:
                DoScriptText(YELL_INTRO_KILL_MADRIGOSA, me);
                ((Creature*)pMadrigosa)->SetCorpseDelay(604800);
                ((Creature*)pMadrigosa)->setDeathState(JUST_DIED);
                IntroPhaseTimer = 8000;
                ++IntroPhase;
                break;
            case 18:
                DoScriptText(YELL_INTRO_TAUNT, me);
                AddSpellToCast(me, SPELL_INTRO_BREAK_ICE);
                if(pInstance)
                    pInstance->SetData(DATA_BRUTALLUS_INTRO_EVENT, DONE);
                else
                    return;
                if(Unit *pTrigger = me->GetUnit(pInstance->GetData64(DATA_BRUTALLUS_TRIGGER)))
                    pTrigger->CastSpell((Unit*)NULL, SPELL_INTRO_BREAK_ICE_KNOCKBACK, false);
                IntroPhaseTimer = 2000;
                ++IntroPhase;
                break;
            case 19:
                EnterEvadeMode();
                ++IntroPhase;
                break;
        }
    }

    void MoveInLineOfSight(Unit *who)
    {
        if (pInstance->GetData(DATA_BRUTALLUS_INTRO_EVENT) == DONE)
            ScriptedAI::MoveInLineOfSight(who);
    }

    void UpdateAI(const uint32 diff)
    {
        if (pInstance->GetData(DATA_BRUTALLUS_INTRO_EVENT) == SPECIAL || pInstance->GetData(DATA_BRUTALLUS_INTRO_EVENT) == IN_PROGRESS || IntroPhase == 19)
        {
            if(IntroPhase < 12 && IntroPhase > 14)
            {
                if (CheckGroundTimer < diff)
                {
                    float x, y, z;
                    me->GetPosition(x, y, z);
                    float ground_z = me->GetMap()->GetHeight(x, y, MAX_HEIGHT, true);
                    if(z > ground_z)
                    me->GetMap()->CreatureRelocation(me, x, y, z, me->GetOrientation());
                    CheckGroundTimer = 500;
                }
                else
                    CheckGroundTimer -= diff;
            }

            if (IntroPhaseTimer < diff)
                DoIntro();
            else
                IntroPhaseTimer -= diff;

            if (IntroPhase >= 7 && IntroPhase <= 9)
            {
                if (IntroFrostBoltTimer < diff)
                {
                    if(Unit *pMadrigosa = me->GetUnit(pInstance->GetData64(DATA_MADRIGOSA)))
                    {
                        pMadrigosa->CastSpell(me, SPELL_INTRO_FROSTBOLT, false);
                        IntroFrostBoltTimer = 2000;
                    }
                }
                else
                    IntroFrostBoltTimer -= diff;
            }

            DoMeleeAttackIfReady();
            CastNextSpellIfAnyAndReady();
            return;
        }

        if (!UpdateVictim())
            return;

        if (CheckTimer < diff)
        {
            DoZoneInCombat();

            me->SetSpeed(MOVE_RUN, 2.0f);
            CheckTimer = 1000;
        }
        else
            CheckTimer -= diff;

        if (SlashTimer < diff)
        {
            AddSpellToCast(me, SPELL_METEOR_SLASH);
            SlashTimer = 11000;
        }
        else
            SlashTimer -= diff;

        if (StompTimer < diff)
        {
            AddSpellToCastWithScriptText(me->getVictim(), SPELL_STOMP, RAND(YELL_LOVE1, YELL_LOVE2, YELL_LOVE3));
            StompTimer = 30000;
        }
        else
            StompTimer -= diff;

        if (BurnTimer < diff)
        {
            if (Unit* pTarget = SelectUnit(SELECT_TARGET_RANDOM, 0))
                AddSpellToCast(pTarget, SPELL_BURN);

            BurnTimer = 20000;
        }
        else
            BurnTimer -= diff;

        if (BerserkTimer < diff && !Enraged)
        {
            AddSpellToCastWithScriptText(me, SPELL_BERSERK, YELL_BERSERK);
            Enraged = true;
        }
        else
            BerserkTimer -= diff;

        CastNextSpellIfAnyAndReady();
        DoMeleeAttackIfReady();
    }
};

struct TRINITY_DLL_DECL npc_death_cloudAI : public ScriptedAI
{
    npc_death_cloudAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = c->GetInstanceData();
    }

    ScriptedInstance* pInstance;
    uint32 SummonTimer;

    void Reset()
    {
        SummonTimer = 1000;
    }

    void JustRespawned()
    {
        ForceSpellCast(me, SPELL_DEATH_CLOUD, INTERRUPT_AND_CAST_INSTANTLY);
        SummonTimer = 1000;
    }

    void UpdateAI(const uint32 diff)
    {
        if (SummonTimer < diff)
        {
            float x, y, z;
            if (Unit* pMadrigosa= me->GetUnit(pInstance->GetData64(DATA_MADRIGOSA)))
            {
                if (!me->IsWithinDist(pMadrigosa, 5.0f))
                {
                    z = me->GetPositionZ();
                    me->GetNearPoint2D(x, y, 5, me->GetAngle(pMadrigosa));
                    me->UpdateAllowedPositionZ(x, y, z);
                    me->SummonCreature(MOB_DEATH_CLOUD, x, y, z, 0, TEMPSUMMON_TIMED_DESPAWN, 10000);
                }
                else
                {
                    if (Unit* pFelmyst= me->GetUnit(pInstance->GetData64(DATA_FELMYST)))
                    {
                        pMadrigosa->GetPosition(x, y, z);
                        pFelmyst->SetVisibility(VISIBILITY_ON);
                        pFelmyst->setFaction(1771);
                        pFelmyst->Relocate(x, y, z);
                        pFelmyst->RemoveUnitMovementFlag(MOVEFLAG_LEVITATING);
                        pFelmyst->GetMotionMaster()->MoveRandom(1);
                    }
                    ((Creature*)pMadrigosa)->RemoveCorpse();
                }
            }
            SummonTimer = 120000000;
        }
        else
            SummonTimer -= diff;
    }
};

struct TRINITY_DLL_DECL brutallus_intro_triggerAI : public Scripted_NoMovementAI
{
    brutallus_intro_triggerAI(Creature *c) : Scripted_NoMovementAI(c)
    {
        pInstance = c->GetInstanceData();
        if(Unit *pMadrigosa = me->GetUnit(pInstance->GetData64(DATA_MADRIGOSA)))
            pMadrigosa->AddUnitMovementFlag(MOVEFLAG_LEVITATING | MOVEFLAG_ONTRANSPORT);
    }

    ScriptedInstance* pInstance;

    void EnterCombat(Unit* /*pWho*/)
    {
        EnterEvadeMode();
    }

    void MoveInLineOfSight(Unit *who)
    {
        if(who->GetTypeId() == TYPEID_PLAYER && me->IsWithinDist(who, 70) && pInstance && pInstance->GetData(DATA_BRUTALLUS_INTRO_EVENT) == NOT_STARTED)
        {
            if(Unit *pBrutallus = me->GetUnit(pInstance->GetData64(DATA_BRUTALLUS)))
                pInstance->SetData(DATA_BRUTALLUS_INTRO_EVENT, SPECIAL);
        }
    }
};

CreatureAI* GetAI_boss_brutallus(Creature *_Creature)
{
    return new boss_brutallusAI (_Creature);
}

CreatureAI* GetAI_npc_death_cloud(Creature *_Creature)
{
    return new npc_death_cloudAI (_Creature);
}

CreatureAI* GetAI_brutallus_intro_trigger(Creature *_Creature)
{
    return new brutallus_intro_triggerAI (_Creature);
}

void AddSC_boss_brutallus()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="npc_death_cloud";
    newscript->GetAI = &GetAI_npc_death_cloud;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="boss_brutallus";
    newscript->GetAI = &GetAI_boss_brutallus;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="brutallus_intro_trigger";
    newscript->GetAI = &GetAI_brutallus_intro_trigger;
    newscript->RegisterSelf();
}
