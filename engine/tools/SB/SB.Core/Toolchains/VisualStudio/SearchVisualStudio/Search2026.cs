using Microsoft.Extensions.FileSystemGlobbing;
using Microsoft.Win32;
using Serilog;
using System.Diagnostics;
using System.Runtime.Versioning;
using System.Text.Json;

namespace SB.Core
{
    [SupportedOSPlatform("windows")]
    public class SearchVS2026
    {
        public class VSInstance
        {
            public string? InstallPath { get; set; }
            public string? InstanceId { get; set; }
            public string? Edition { get; set; }
            public string? Version { get; set; }
            public string? VCVarsAllBat { get; set; }
            public string? VCVarsBat { get; set; }
            public string? WindowsSDKBat { get; set; }
            public bool IsValid => !string.IsNullOrEmpty(InstallPath) && Directory.Exists(InstallPath);
        }

        private static readonly ILogger Log = Serilog.Log.ForContext<SearchVS2026>();

        /// <summary>
        /// 从注册表查找 VS2026 安装实例
        /// </summary>
        public static List<VSInstance> FindFromRegistry()
        {
            var instances = new List<VSInstance>();

            try
            {
                var registryPaths = new[]
                {
                    @"SOFTWARE\WOW6432Node\Microsoft\VisualStudio",
                    @"SOFTWARE\Microsoft\VisualStudio",
                    @"SOFTWARE\WOW6432Node\Microsoft\DevDiv\vs\Servicing\18.0",
                    @"SOFTWARE\Microsoft\DevDiv\vs\Servicing\18.0"
                };

                foreach (var path in registryPaths)
                {
                    try
                    {
                        using var key = Registry.LocalMachine.OpenSubKey(path);
                        if (key == null) continue;

                        foreach (var subKeyName in key.GetSubKeyNames())
                        {
                            // VS2026 的版本号以 18 开头
                            if (!subKeyName.StartsWith("18.") && !subKeyName.Contains("_")) continue;

                            using var subKey = key.OpenSubKey(subKeyName);
                            if (subKey == null) continue;

                            var installPath = subKey.GetValue("InstallLocation") as string ??
                                            subKey.GetValue("ProductDir") as string;

                            if (!string.IsNullOrEmpty(installPath) && Directory.Exists(installPath))
                            {
                                var instance = new VSInstance
                                {
                                    InstallPath = Path.GetFullPath(installPath),
                                    InstanceId = subKeyName,
                                    Version = subKey.GetValue("Version") as string ?? "18.0"
                                };

                                FindBatchFiles(instance);

                                if (instance.IsValid)
                                {
                                    instances.Add(instance);
                                    Log.Verbose("Found VS2026 from registry: {InstallPath}", instance.InstallPath);
                                }
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        Log.Verbose("Registry search failed for path {Path}: {Message}", path, ex.Message);
                    }
                }

                FindFromUninstallRegistry(instances);
            }
            catch (Exception ex)
            {
                Log.Warning("Failed to search registry for VS2026: {Message}", ex.Message);
            }

            return instances;
        }

        private static void FindFromUninstallRegistry(List<VSInstance> instances)
        {
            var uninstallPaths = new[]
            {
                @"SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall",
                @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall"
            };

            foreach (var path in uninstallPaths)
            {
                try
                {
                    using var key = Registry.LocalMachine.OpenSubKey(path);
                    if (key == null) continue;

                    foreach (var subKeyName in key.GetSubKeyNames())
                    {
                        using var subKey = key.OpenSubKey(subKeyName);
                        if (subKey == null) continue;

                        var displayName = subKey.GetValue("DisplayName") as string;
                        if (string.IsNullOrEmpty(displayName) || !displayName.Contains("Visual Studio") || !displayName.Contains("2026"))
                            continue;

                        var installLocation = subKey.GetValue("InstallLocation") as string;
                        if (string.IsNullOrEmpty(installLocation) || !Directory.Exists(installLocation))
                            continue;

                        if (instances.Any(i => i.InstallPath == installLocation))
                            continue;

                        var instance = new VSInstance
                        {
                            InstallPath = Path.GetFullPath(installLocation),
                            InstanceId = subKeyName,
                            Edition = ExtractEdition(displayName),
                            Version = subKey.GetValue("DisplayVersion") as string ?? "18.0"
                        };

                        FindBatchFiles(instance);

                        if (instance.IsValid)
                        {
                            instances.Add(instance);
                            Log.Verbose("Found VS2026 from uninstall registry: {InstallPath}", instance.InstallPath);
                        }
                    }
                }
                catch (Exception ex)
                {
                    Log.Verbose("Uninstall registry search failed for path {Path}: {Message}", path, ex.Message);
                }
            }
        }

        public static List<VSInstance> FindFromVSWhere()
        {
            var instances = new List<VSInstance>();

            try
            {
                var vswherePaths = new[]
                {
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),
                               "Microsoft Visual Studio", "Installer", "vswhere.exe"),
                    Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),
                               "Microsoft Visual Studio", "Installer", "vswhere.exe"),
                    "vswhere.exe"
                };

                string? vswherePath = vswherePaths.FirstOrDefault(File.Exists);

                if (string.IsNullOrEmpty(vswherePath))
                {
                    Log.Verbose("vswhere.exe not found");
                    return instances;
                }

                Log.Verbose("Using vswhere at: {Path}", vswherePath);

                // 执行 vswhere 查找 VS2026 (版本 18.x)
                var process = new Process
                {
                    StartInfo = new ProcessStartInfo
                    {
                        FileName = vswherePath,
                        Arguments = "-version \"[18.0,19.0)\" -format json -utf8 -nologo",
                        UseShellExecute = false,
                        RedirectStandardOutput = true,
                        RedirectStandardError = true,
                        CreateNoWindow = true
                    }
                };

                process.Start();
                string output = process.StandardOutput.ReadToEnd();
                process.WaitForExit();

                if (!string.IsNullOrEmpty(output))
                {
                    var jsonInstances = JsonSerializer.Deserialize<List<VSWhereInstance>>(output);
                    if (jsonInstances != null)
                    {
                        foreach (var jsonInstance in jsonInstances)
                        {
                            if (string.IsNullOrEmpty(jsonInstance.installationPath)) continue;

                            var instance = new VSInstance
                            {
                                InstallPath = Path.GetFullPath(jsonInstance.installationPath),
                                InstanceId = jsonInstance.instanceId,
                                Edition = jsonInstance.installationName,
                                Version = jsonInstance.installationVersion
                            };

                            FindBatchFiles(instance);

                            if (instance.IsValid)
                            {
                                instances.Add(instance);
                                Log.Verbose("Found VS2026 from vswhere: {InstallPath}", instance.InstallPath);
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Warning("Failed to use vswhere: {Message}", ex.Message);
            }

            return instances;
        }

        public static List<VSInstance> FindFromFileSystem()
        {
            var instances = new List<VSInstance>();

            try
            {
                var programFilesPaths = GetProgramFilesPaths();

                foreach (var programFiles in programFilesPaths)
                {
                    var vsBasePath = Path.Combine(programFiles, "Microsoft Visual Studio", "18");

                    if (!Directory.Exists(vsBasePath))
                        continue;

                    foreach (var editionDir in Directory.GetDirectories(vsBasePath))
                    {
                        var instance = new VSInstance
                        {
                            InstallPath = Path.GetFullPath(editionDir),
                            Edition = Path.GetFileName(editionDir),
                            Version = "18.0"
                        };

                        FindBatchFiles(instance);

                        if (instance.IsValid)
                        {
                            instances.Add(instance);
                            Log.Verbose("Found VS2026 from file system: {InstallPath}", instance.InstallPath);
                        }
                    }
                }

                if (instances.Count == 0)
                {
                    instances.AddRange(FindFromAllDrives());
                }
            }
            catch (Exception ex)
            {
                Log.Warning("Failed to search file system: {Message}", ex.Message);
            }

            return instances;
        }

        private static List<VSInstance> FindFromAllDrives()
        {
            var instances = new List<VSInstance>();
            var drives = DriveInfo.GetDrives().Where(d => d.DriveType == DriveType.Fixed && d.IsReady);

            foreach (var drive in drives)
            {
                var searchPaths = new[]
                {
                    Path.Combine(drive.RootDirectory.FullName, "Program Files", "Microsoft Visual Studio", "18"),
                    Path.Combine(drive.RootDirectory.FullName, "Program Files (x86)", "Microsoft Visual Studio", "18")
                };

                foreach (var searchPath in searchPaths)
                {
                    if (!Directory.Exists(searchPath))
                        continue;

                    try
                    {
                        foreach (var editionDir in Directory.GetDirectories(searchPath))
                        {
                            var instance = new VSInstance
                            {
                                InstallPath = Path.GetFullPath(editionDir),
                                Edition = Path.GetFileName(editionDir),
                                Version = "18.0"
                            };

                            FindBatchFiles(instance);

                            if (instance.IsValid && !instances.Any(i => i.InstallPath == instance.InstallPath))
                            {
                                instances.Add(instance);
                                Log.Verbose("Found VS2026 from drive {Drive}: {InstallPath}", drive.Name, instance.InstallPath);
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        Log.Verbose("Failed to search {Path}: {Message}", searchPath, ex.Message);
                    }
                }
            }

            return instances;
        }

        private static void FindBatchFiles(VSInstance instance)
        {
            if (string.IsNullOrEmpty(instance.InstallPath) || !Directory.Exists(instance.InstallPath))
                return;

            try
            {
                var matcher = new Matcher();
                matcher.AddIncludePatterns(new[]
                {
                    "**/vcvarsall.bat",
                    "**/vsdevcmd/ext/vcvars.bat",
                    "**/vsdevcmd/core/winsdk.bat"
                });

                var files = matcher.GetResultsInFullPath(instance.InstallPath);

                foreach (var file in files)
                {
                    var fileName = Path.GetFileName(file);
                    switch (fileName.ToLowerInvariant())
                    {
                        case "vcvarsall.bat":
                            instance.VCVarsAllBat = file;
                            break;
                        case "vcvars.bat":
                            instance.VCVarsBat = file;
                            break;
                        case "winsdk.bat":
                            instance.WindowsSDKBat = file;
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Verbose("Failed to find batch files in {Path}: {Message}", instance.InstallPath, ex.Message);
            }
        }

        private static List<string> GetProgramFilesPaths()
        {
            var paths = new HashSet<string>();

            var envPaths = new[]
            {
                Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),
                Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),
                Environment.GetEnvironmentVariable("ProgramFiles"),
                Environment.GetEnvironmentVariable("ProgramFiles(x86)"),
                Environment.GetEnvironmentVariable("ProgramW6432")
            };

            foreach (var path in envPaths)
            {
                if (!string.IsNullOrEmpty(path) && Directory.Exists(path))
                {
                    paths.Add(Path.GetFullPath(path));
                }
            }

            return paths.ToList();
        }

        private static string ExtractEdition(string displayName)
        {
            if (displayName.Contains("Community")) return "Community";
            if (displayName.Contains("Professional")) return "Professional";
            if (displayName.Contains("Enterprise")) return "Enterprise";
            if (displayName.Contains("Preview")) return "Preview";
            return "Unknown";
        }

        public static VSInstance? FindBestInstance()
        {
            Log.Information("Searching for Visual Studio 2026...");

            var allInstances = new List<VSInstance>();

            var vswhereInstances = FindFromVSWhere();
            allInstances.AddRange(vswhereInstances);
            Log.Verbose("Found {Count} instances from vswhere", vswhereInstances.Count);

            if (allInstances.Count == 0)
            {
                var registryInstances = FindFromRegistry();
                allInstances.AddRange(registryInstances);
                Log.Verbose("Found {Count} instances from registry", registryInstances.Count);
            }

            if (allInstances.Count == 0)
            {
                var fileSystemInstances = FindFromFileSystem();
                allInstances.AddRange(fileSystemInstances);
                Log.Verbose("Found {Count} instances from file system", fileSystemInstances.Count);
            }

            var uniqueInstances = allInstances
                .GroupBy(i => i.InstallPath?.ToLowerInvariant())
                .Select(g => g.First())
                .ToList();

            if (uniqueInstances.Count == 0)
            {
                Log.Warning("No Visual Studio 2026 installation found");
                return null;
            }

            var bestInstance = uniqueInstances
                .OrderByDescending(i => !string.IsNullOrEmpty(i.VCVarsBat) && !string.IsNullOrEmpty(i.WindowsSDKBat))
                .ThenByDescending(i => !string.IsNullOrEmpty(i.VCVarsAllBat))
                .ThenByDescending(i => i.Edition == "Enterprise")
                .ThenByDescending(i => i.Edition == "Professional")
                .ThenByDescending(i => i.Edition == "Community")
                .FirstOrDefault();

            if (bestInstance != null)
            {
                Log.Information("Selected VS2026: {Edition} at {Path}", bestInstance.Edition, bestInstance.InstallPath);
            }

            return bestInstance;
        }

        private class VSWhereInstance
        {
            public string? instanceId { get; set; }
            public string? installDate { get; set; }
            public string? installationName { get; set; }
            public string? installationPath { get; set; }
            public string? installationVersion { get; set; }
            public string? productId { get; set; }
            public string? productPath { get; set; }
            public bool isPrerelease { get; set; }
        }
    }
}
