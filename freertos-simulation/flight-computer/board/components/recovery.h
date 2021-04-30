
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// UMSATS Rocketry 2019
//
// Repository:
//  UMSATS/Avionics
//
// File Description:
//  Header file for functions related to recovery ignition circuit.
//
// History
// 2019-05-29 by Joseph Howarth
// - Created.
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifndef RECOVERY_H
#define RECOVERY_H
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// INCLUDES
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// DEFINITIONS AND MACROS
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <inttypes.h>


#define EMATCH_ON_TIME                50

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// ENUMS AND ENUM TYPEDEFS
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef enum
{
    RecoverSelectDrogueParachute,
    RecoverySelectMainParachute,
    RecoverySelectCount
} RecoverySelect;

typedef enum RecoveryContinuityStatus
{
    RecoveryContinuityStatusOpenCircuit,
    RecoveryContinuityStatusShortCircuit,
    RecoveryContinuityStatusCount
} RecoveryContinuityStatus;

typedef enum
{
    RecoveryOverCurrentStatusNoOverCurrent,
    RecoveryOverCurrentStatusOverCurrent,
    RecoveryOverCurrentStatusCount
} RecoveryOverCurrentStatus;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
//  Sets up the GPIO pins for the recovery functions.
//
// Returns:
//  Enter description of return values (if any).
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
void recovery_init ( );

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
//  Enables the mosfet driver for the specified recovery event .
//
// Returns:
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
void recoveryEnableMOSFET ( RecoverySelect recoverySelect );
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
//  Activates the mosfet driver for the specified recovery event.
//        The driver will be activated for the number of ms specified by the constant EMATCH_ON_TIME.
//  The driver will be disabled after and must be re-enabled be fore every call of this function.
//
// Returns:
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
void recoveryActivateMOSFET ( RecoverySelect recoverySelect );
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
//  Checks the continuity for the specified recovery circuit .
//
// Returns:
//        A RecoveryContinuityStatus of RecoveryContinuityStatusOpenCircuit or RecoveryContinuityStatusShortCircuit.
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
RecoveryContinuityStatus recoveryCheckContinuity ( RecoverySelect recoverySelect );
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
//  Checks the overcurrent flag for the specified recovery circuit .
//
// Returns:
//        A RecoveryContinuityStatus of RecoveryOverCurrentStatusNoOverCurrent or RecoveryOverCurrentStatusOverCurrent.
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
RecoveryOverCurrentStatus recoveryCheckOverCurrent ( RecoverySelect recoverySelect );

#endif // RECOVERY_H
