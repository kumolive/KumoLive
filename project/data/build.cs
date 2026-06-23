using SB;
using SB.Core;

[TargetScript]
public static class KumoData
{
    static KumoData()
    {
        var Target = Engine.Module("KumoData")
            .EnableUnityBuild()
            .OptimizationLevel(OptimizationLevel.Fastest)

            // Engine dependency
            .Depend(Visibility.Public, "SkrRuntime")

            // Packages
            .Require("nlohmann_json", new PackageConfig { Version = new Version(3, 11, 3) })
            .Depend(Visibility.Private, "nlohmann_json@nlohmann_json")
            .Require("sqlite3", new PackageConfig { Version = new Version(3, 45, 0) })
            .Depend(Visibility.Private, "sqlite3@sqlite3")

            // Include directories
            .IncludeDirs(Visibility.Public, "types/include")
            .IncludeDirs(Visibility.Public, "messages/include")
            .IncludeDirs(Visibility.Public, "events/include")
            .IncludeDirs(Visibility.Public, "config/include")
            .IncludeDirs(Visibility.Public, "database/include")
            .IncludeDirs(Visibility.Public, "bilibili/include")
            .IncludeDirs(Visibility.Public, "tts/include")
            .IncludeDirs(Visibility.Public, "update/include")

            // Source files
            .AddCppFiles("types/src/*.cpp")
            .AddCppFiles("messages/src/*.cpp")
            .AddCppFiles("events/src/*.cpp")
            .AddCppFiles("config/src/*.cpp")
            .AddCppFiles("database/src/*.cpp")
            .AddCppFiles("bilibili/src/*.cpp")
            .AddCppFiles("tts/src/*.cpp")
            .AddCppFiles("update/src/*.cpp");
    }
}
