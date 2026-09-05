using UnrealBuildTool;
using System.Collections.Generic;

public class FutebolAviaoTarget : TargetRules
{
	public FutebolAviaoTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("FutebolAviao");
	}
}
