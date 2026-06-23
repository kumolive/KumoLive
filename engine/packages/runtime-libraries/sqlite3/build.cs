using SB;
using SB.Core;

[TargetScript(TargetCategory.Package)]
public static class SQLite3
{
    static SQLite3()
    {
        BuildSystem.Package("sqlite3")
            .AddTarget("sqlite3", (Target Target, PackageConfig Config) =>
            {
                if (Config.Version != new Version(3, 45, 0))
                    throw new TaskFatalError("sqlite3 version mismatch!");
                Target.TargetType(TargetType.Static)
                    .IncludeDirs(Visibility.Public, ".")
                    .FpModel(FpModel.Fast)
                    .AddCFiles("sqlite3.c");
            });
    }
}
