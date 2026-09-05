#include "FutebolAviaoGameModeBase.h"
#include "Flight/PlanePawn.h"

AFutebolAviaoGameModeBase::AFutebolAviaoGameModeBase()
{
	DefaultPawnClass = APlanePawn::StaticClass();
}
