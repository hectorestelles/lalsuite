/* Copyright (C) 2012 Evan Ochsner
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with with program; see the file COPYING. If not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 */

#ifndef _LALSIMINSPIRALWAVEFORMFLAGS_H
#define _LALSIMINSPIRALWAVEFORMFLAGS_H

#include <lal/LALMalloc.h>
#include <lal/LALError.h>

/** Default values for all enumerated flags */ 
#define LAL_SIM_INSPIRAL_INTERACTION_DEFAULT LAL_SIM_INSPIRAL_INTERACTION_ALL
#define LAL_SIM_INSPIRAL_FRAME_AXIS_DEFAULT LAL_SIM_INSPIRAL_FRAME_AXIS_VIEW
#define LAL_SIM_INSPIRAL_MODES_CHOICE_DEFAULT LAL_SIM_INSPIRAL_MODES_CHOICE_RESTRICTED

/** 
 * Enumeration to specify which interaction will be used in the waveform
 * generation. Their combination also can be used by the bitwise or.
 */
typedef enum {
    LAL_SIM_INSPIRAL_INTERACTION_NONE = 0, /**< No spin, tidal or other interactions */
    LAL_SIM_INSPIRAL_INTERACTION_SPIN_ORBIT_15PN = 1, /**< Leading order spin-orbit interaction */
    LAL_SIM_INSPIRAL_INTERACTION_SPIN_SPIN_2PN = 1 << 1,  /**< Spin-spin interaction */
    LAL_SIM_INSPIRAL_INTERACTION_SPIN_SPIN_SELF_2PN = 1 << 2,     /**<  Spin-spin-self interaction */
    LAL_SIM_INSPIRAL_INTERACTION_QUAD_MONO_2PN = 1 << 3,     /**< Quadrupole-monopole interaction */
    LAL_SIM_INSPIRAL_INTERACTION_SPIN_ORBIT_25PN = 1 << 4,     /**<  Next-to-leading-order spin-orbit interaction */
    LAL_SIM_INSPIRAL_INTERACTION_SPIN_ORBIT_3PN = 1 << 5,  /**< Spin-spin interaction */
    LAL_SIM_INSPIRAL_INTERACTION_TIDAL_5PN = 1 << 6, /**< Leading-order tidal interaction */
    LAL_SIM_INSPIRAL_INTERACTION_TIDAL_6PN = 1 << 7, /**< Next-to-leading-order tidal interaction */
    LAL_SIM_INSPIRAL_INTERACTION_ALL_SPIN = (1 << 6) - 1, /**< all spin interactions, no tidal interactions */
    LAL_SIM_INSPIRAL_INTERACTION_ALL = (1 << 8) - 1 /**< all spin and tidal interactions */
} LALSimInspiralInteraction;

/**
 * Enumerator for choosing the reference frame associated with
 * PSpinInspiralRD waveforms.
 */
typedef enum {
    LAL_SIM_INSPIRAL_FRAME_AXIS_VIEW, /**< Set z-axis along direction of GW propagation (line of sight) */
    LAL_SIM_INSPIRAL_FRAME_AXIS_TOTAL_J, /**< Set z-axis along the initial total angular momentum */
    LAL_SIM_INSPIRAL_FRAME_AXIS_ORBITAL_L, /**< Set z-axis along the initial orbital angular momentum */
} LALSimInspiralFrameAxis;

/** 
 * Enumerator for choosing which modes to include in IMR models.
 *
 * 'ALL' means to use all modes available to that model.
 *
 * 'RESTRICTED' means only the (2,2) mode for non-precessing models
 * or only the set of l=2 modes for precessing models.
 */
typedef enum {
    LAL_SIM_INSPIRAL_MODES_CHOICE_RESTRICTED, /**< Include only (2,2) or l=2 modes */
    LAL_SIM_INSPIRAL_MODES_CHOICE_ALL /**< Include all available (l,m) modes */
} LALSimInspiralModesChoice;


/**
 * Struct containing several enumerated flags that control specialized behavior
 * for some waveform approximants.
 * 
 * Users: Access this struct only through the Create/Destroy/Set/Get/IsDefault
 * functions declared in this file.
 * 
 * Developers: Do not add anything but enumerated flags to this struct. Avoid
 * adding extra flags whenever possible.
 */
typedef struct tagLALSimInspiralWaveformFlags
{
    LALSimInspiralInteraction interactionChoice; /**< Flag to control spin/tidal effects */
    LALSimInspiralFrameAxis axisChoice; /**< Flag to set frame z-axis convention */
    LALSimInspiralModesChoice modesChoice; /**< Flag to control which modes are included in IMR models */
} LALSimInspiralWaveformFlags;

/**
 * Create a new LALSimInspiralWaveformFlags struct 
 * with all flags set to their default values.
 * 
 * If you create a struct, remember to destroy it when you are done with it.
 */
LALSimInspiralWaveformFlags *XLALSimInspiralCreateWaveformFlags(void);

/**
 * Destroy a LALSimInspiralWaveformFlags struct.
 */
void XLALSimInspiralDestroyWaveformFlags(
        LALSimInspiralWaveformFlags *waveFlags
        );

/**
 * Returns 1 if all fields of LALSimInspiralWaveformFlags have default value, 
 * returns 0 otherwise.
 */
int XLALSimInspiralWaveformFlagsIsDefault(
        LALSimInspiralWaveformFlags *waveFlags
        );

/**
 * Set the LALSimInspiralInteraction within a LALSimInspiralWaveformFlags struct
 */
void XLALSimInspiralSetInteraction(
        LALSimInspiralWaveformFlags *waveFlags, /**< Struct whose flag will be set */
        LALSimInspiralInteraction interactionChoice /**< value to set flag to */
        );

/**
 * Get the LALSimInspiralInteraction within a LALSimInspiralWaveformFlags struct
 */
LALSimInspiralInteraction XLALSimInspiralGetInteraction(
        LALSimInspiralWaveformFlags *waveFlags
        );

/**
 * Returns 1 if LALSimInspiralInteraction has default value, 0 otherwise
 */
int XLALSimInspiralInteractionIsDefault(
        LALSimInspiralInteraction interactionChoice
        );

/**
 * Set the LALSimInspiralFrameAxis within a LALSimInspiralWaveformFlags struct
 */
void XLALSimInspiralSetFrameAxis(
        LALSimInspiralWaveformFlags *waveFlags, /**< Struct whose flag will be set */
        LALSimInspiralFrameAxis axisChoice /**< value to set flag to */
        );

/**
 * Get the LALSimInspiralFrameAxis within a LALSimInspiralWaveformFlags struct
 */
LALSimInspiralFrameAxis XLALSimInspiralGetFrameAxis(
        LALSimInspiralWaveformFlags *waveFlags
        );

/**
 * Returns 1 if LALSimInspiralFrameAxis has default value, 0 otherwise
 */
int XLALSimInspiralFrameAxisIsDefault(
        LALSimInspiralFrameAxis axisChoice
        );

/**
 * Set the LALSimInspiralModesChoice within a LALSimInspiralWaveformFlags struct
 */
void XLALSimInspiralSetModesChoice(
        LALSimInspiralWaveformFlags *waveFlags, /**< Struct whose flag will be set */
        LALSimInspiralModesChoice modesChoice /**< value to set flag to */
        );

/**
 * Get the LALSimInspiralModesChoice within a LALSimInspiralWaveformFlags struct
 */
LALSimInspiralModesChoice XLALSimInspiralGetModesChoice(
        LALSimInspiralWaveformFlags *waveFlags
        );

/**
 * Returns 1 if LALSimInspiralModesChoice has default value, 0 otherwise
 */
int XLALSimInspiralModesChoiceIsDefault(
        LALSimInspiralModesChoice modesChoice
        );

#endif /* _LALSIMINSPIRALWAVEFORMFLAGS_H */
