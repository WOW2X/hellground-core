/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Creature.h"
#include "MapManager.h"
#include "RandomMovementGenerator.h"
#include "DestinationHolderImp.h"
#include "Map.h"
#include "Util.h"

template<>
void
RandomMovementGenerator<Creature>::_setRandomLocation(Creature &creature)
{
    float X,Y,Z,z,nx,ny,nz,wander_distance,ori,dist;

    creature.GetRespawnCoord(X, Y, Z, &ori, &wander_distance);

    z = creature.GetPositionZ();
    Map const* map = creature.GetBaseMap();

    // For 2D/3D system selection
    //bool is_land_ok  = creature.canWalk();                // not used?
    //bool is_water_ok = creature.canSwim();                // not used?
    bool is_air_ok   = creature.canFly();

    const float angle = rand_norm()*(M_PI*2);
    const float range = rand_norm()*wander_distance;
    const float distanceX = range * cos(angle);
    const float distanceY = range * sin(angle);

    nx = X + distanceX;
    ny = Y + distanceY;

    // prevent invalid coordinates generation
    Trinity::NormalizeMapCoord(nx);
    Trinity::NormalizeMapCoord(ny);

    dist = distanceX*distanceX + distanceY*distanceY;

    if (is_air_ok)                                          // 3D system above ground and above water (flying mode)
    {
        const float distanceZ = rand_norm() * sqrtf(dist)/2;// Limit height change
        nz = Z + distanceZ;
        float tz = map->GetHeight(nx, ny, nz-2.0f, false);  // Map check only, vmap needed here but need to alter vmaps checks for height.
        float wz = map->GetWaterLevel(nx, ny);

        // Problem here, we must fly above the ground and water, not under. Let's try on next tick
        if (tz >= nz || wz >= nz)
            return;
    }
    //else if (is_water_ok)                                 // 3D system under water and above ground (swimming mode)
    else                                                    // 2D only
    {
        dist = dist >= 100.0f ? 10.0f : sqrtf(dist);        // 10.0 is the max that vmap high can check (MAX_CAN_FALL_DISTANCE)

        // The fastest way to get an accurate result 90% of the time.
        // Better result can be obtained like 99% accuracy with a ray light, but the cost is too high and the code is too long.
        nz = map->GetHeight(nx, ny, Z+dist-2.0f, false);

        if (fabs(nz-Z) > dist)                              // Map check
        {
            nz = map->GetHeight(nx, ny, Z-2.0f, true);      // Vmap Horizontal or above

            if (fabs(nz-Z) > dist)
            {
                // Vmap Higher
                nz = map->GetHeight(nx, ny, Z+dist-2.0f, true);

                // let's forget this bad coords where a z cannot be find and retry at next tick
                if (fabs(nz-Z) > dist)
                    return;
            }
        }
    }

    Traveller<Creature> traveller(creature);

    creature.SetOrientation(creature.GetAngle(nx, ny));
    i_destinationHolder.SetDestination(traveller, nx, ny, nz);
    creature.addUnitState(UNIT_STAT_ROAMING);

    if (is_air_ok)
    {
        i_nextMoveTime.Reset(i_destinationHolder.GetTotalTravelTime());
        creature.AddUnitMovementFlag(MOVEMENTFLAG_FLYING2);
    }
    //else if (is_water_ok)                                 // Swimming mode to be done with more than this check
    else
    {
        i_nextMoveTime.Reset(urand(500+i_destinationHolder.GetTotalTravelTime(), 10000+i_destinationHolder.GetTotalTravelTime()));
        creature.AddUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);
    }
}

template<>
void
RandomMovementGenerator<Creature>::Initialize(Creature &creature)
{
    if (!creature.isAlive())
        return;

    if (creature.canFly())
        creature.AddUnitMovementFlag(MOVEMENTFLAG_FLYING2);
    else
        creature.AddUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);

    _setRandomLocation(creature);
}

template<>
void
RandomMovementGenerator<Creature>::Reset(Creature &creature)
{
    Initialize(creature);
}

template<>
bool
RandomMovementGenerator<Creature>::Update(Creature &creature, const uint32 &diff)
{
    if (creature.hasUnitState(UNIT_STAT_ROOT | UNIT_STAT_STUNNED | UNIT_STAT_DISTRACTED | UNIT_STAT_DIED))
    {
        i_nextMoveTime.Update(i_nextMoveTime.GetExpiry());  // Expire the timer
        creature.clearUnitState(UNIT_STAT_ROAMING);
        return true;
    }

    i_nextMoveTime.Update(diff);

    if (i_destinationHolder.HasArrived() && !creature.IsStopped() && !creature.canFly())
        creature.clearUnitState(UNIT_STAT_ROAMING);

    if (!i_destinationHolder.HasArrived() && creature.IsStopped())
        creature.addUnitState(UNIT_STAT_ROAMING);

    CreatureTraveller traveller(creature);

    if (i_destinationHolder.UpdateTraveller(traveller, diff, true))
    {
        if (i_nextMoveTime.Passed())
        {
            if (creature.canFly())
                creature.AddUnitMovementFlag(MOVEMENTFLAG_FLYING2);
            else
                creature.AddUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);

            _setRandomLocation(creature);
        }
        else if (creature.isPet() && creature.GetOwner() && !creature.IsWithinDist(creature.GetOwner(), PET_FOLLOW_DIST+2.5f))
        {
           creature.AddUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);
           _setRandomLocation(creature);
        }
    }
    return true;
}
