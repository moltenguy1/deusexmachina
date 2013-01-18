﻿using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Security.Cryptography;

namespace ThirdPartyHelper
{
    class Program
    {
        static void Main(string[] args)
        {
            var defaultColor = Console.ForegroundColor;
            if (args == null || args.Length == 0)
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("WARNING. Command line must contain a file name.");
                Console.ForegroundColor = defaultColor;
                return;
            }

            try
            {
                var downloadCaheFolder = args.Length > 1 ? args[1] : null;
                if (downloadCaheFolder != null)
                {
                    try
                    {
                        var folder = Path.GetFullPath(downloadCaheFolder);
                        if (!Directory.Exists(folder))
                            Directory.CreateDirectory(folder);
                        downloadCaheFolder = folder;
                    }
                    catch (Exception ex)
                    {
                        Console.ForegroundColor = ConsoleColor.Yellow;
                        Console.WriteLine("WARNING. Argument '{0}' is not valid: {1}.", downloadCaheFolder, ex.Message);
                        Console.ForegroundColor = defaultColor;
                        downloadCaheFolder = null;
                    }
                }
                DownloadAndExtract(args[0], downloadCaheFolder);
            }
            catch(Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("ERROR. {0}", ex);
                Console.ForegroundColor = defaultColor;
            }

#if DEBUG
            Console.WriteLine("Press any key to quit.");
            Console.ReadKey(false);
#endif
        }

        static void DownloadAndExtract(string fileName, string caheFolder)
        {   
            var file = Path.GetFullPath(fileName);
            if (!File.Exists(file))
                throw new FileNotFoundException("File is not found.", file);

            var downloadList=new List<DownloadInfo>();
            using(var stream=new FileStream(file,FileMode.Open,FileAccess.Read))
            {
                var reader = new StreamReader(stream);
                while(!reader.EndOfStream)
                {
                    var line = reader.ReadLine();
                    if (string.IsNullOrEmpty(line))
                        continue;

                    var downloadInfo = DownloadInfo.Parse(line);
                    downloadList.Add(downloadInfo);
                }
            }

            var shell = new ShellClass();
            var md5 = MD5.Create();

            foreach (var item in downloadList)
            {
                bool downloaded = false, keepFile = false;
                string tempFile;
                if (caheFolder != null && !string.IsNullOrEmpty(item.CacheFileName))
                {
                    tempFile = Path.GetFullPath(Path.Combine(caheFolder, item.CacheFileName));
                    var tempDir = Path.GetDirectoryName(tempFile);
                    Debug.Assert(!string.IsNullOrEmpty(tempDir));
                    if (!Directory.Exists(tempDir))
                        Directory.CreateDirectory(tempDir);
                    else if (File.Exists(tempFile))
                    {
                        using (var fStream=new FileStream(tempFile,FileMode.Open,FileAccess.Read))
                        {
                            var hash = md5.ComputeHash(fStream);
                            var hashString = BitConverter.ToString(hash).Replace("-", string.Empty).ToLowerInvariant();
                            downloaded = hashString == item.HashCode;
                            if (downloaded)
                                Console.WriteLine("The file from {0} has been found in the cache.", item.Uri);
                        }
                    }
                    keepFile = true;
                }
                else
                    tempFile = Path.GetTempFileName() + ".zip";

                if (!downloaded)
                {
                    HttpGet(item.Uri, tempFile);

                    using (var fStream = new FileStream(tempFile, FileMode.Open, FileAccess.Read))
                    {
                        var hash = md5.ComputeHash(fStream);
                        var hashString = BitConverter.ToString(hash).Replace("-", string.Empty).ToLowerInvariant();
                        var color = Console.ForegroundColor;
                        Console.ForegroundColor = ConsoleColor.Yellow;
                        if (string.IsNullOrEmpty(item.HashCode))
                        {
                            Console.WriteLine(
                                "WARNING. Hash code is not defined for the file. Hash code check will be skipped.");
                            Console.WriteLine("Hash code: {0}", hashString);
                        }
                        else if (!string.Equals(item.HashCode, hashString, StringComparison.InvariantCultureIgnoreCase))
                        {
                            Console.WriteLine("WARNING. Hash code mismatch.");
                            Console.WriteLine("Hash code: {0}. Required hash code: {1}.", hashString, item.HashCode);
                        }
                        Console.ForegroundColor = color;
                    }
                }

                UnzipAll(shell, tempFile, item.SourcePath, item.DestinationPath);

                if (!keepFile)
                {
                    try
                    {
                        File.Delete(tempFile);
                    }
                    catch
                    {
                        var color = Console.ForegroundColor;
                        Console.ForegroundColor = ConsoleColor.Yellow;
                        Console.WriteLine("WARNING. Can't delete temporary file '{0}'.", tempFile);
                        Console.ForegroundColor = color;
                    }
                }
            }

        }

        static void HttpGet(string uri, string destination)
        {
            Console.WriteLine("Downloading file from {0}", uri);
            var request = (HttpWebRequest)WebRequest.Create(uri);
            request.Method = WebRequestMethods.Http.Get;
            
            using (var response = (HttpWebResponse)request.GetResponse())
            using (var getStream = response.GetResponseStream())
            {
                if (getStream == null)
                    throw new InvalidOperationException(string.Format("There was no response from '{0}'.", uri));

                using (var fStream = new FileStream(destination, FileMode.Create, FileAccess.Write))
                {
                    var buffer = new byte[2048];
                    int length;
                    do
                    {
                        
                        length = getStream.Read(buffer, 0, buffer.Length);
                        fStream.Write(buffer, 0, length);
                    } while (length > 0);
                }
            }
        }

        private static void UnzipAll(ShellClass shell, string zippedFile, string sourcePath, string destinationFolder)
        {
            var destinationPath = Path.GetFullPath(destinationFolder);
            Console.WriteLine("Extracting files to {0}", destinationPath);

            if (!Directory.Exists(destinationPath))
                Directory.CreateDirectory(destinationPath);

            var source = Path.Combine(Path.GetFullPath(zippedFile), sourcePath);

            var from = shell.NameSpace(source);
            var to = shell.NameSpace(destinationPath);

            to.CopyHere(from.Items(), 0);
        }
    }
}
