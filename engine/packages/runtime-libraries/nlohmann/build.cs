using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class NlohmannJson
{
    static NlohmannJson()
    {
        BuildSystem.Package("nlohmann_json")
            .AddTarget("nlohmann_json", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(3, 11, 3))
                    throw new TaskFatalError("nlohmann_json version mismatch!");
                Target.TargetType(TargetType.HeaderOnly)
                    .CppVersion("17")
                    .IncludeDirs(Visibility.Public, ".");
            });
    }
}
