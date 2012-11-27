﻿using System;
using System.IO;
using System.Linq;

namespace CreatorIDE.Package
{
    public class CidePathHelper
    {
        public const char ScopeSeparatorChar = ':';

        public static bool IsInScope(string path)
        {
            if (path == null)
                throw new ArgumentNullException("path");

            var idx = path.IndexOf(ScopeSeparatorChar);
            if (idx < 0)
                return false;

            var scope = path.Substring(idx).Trim();
            if (scope.Length < 2)
                return false; // It's just a drive letter or an empty string

            return scope.All(ch => ch != Path.DirectorySeparatorChar && ch != Path.AltDirectorySeparatorChar);
        }

        public static string AddScope(string scope, string path)
        {
            if(scope==null)
                throw new ArgumentNullException("scope");
            if(path==null)
                throw new ArgumentNullException("path");
            scope = scope.Trim();
            if(scope.Length<2)
                throw new ArgumentException(SR.GetString(SR.ScopeNameIsTooShort), "scope");
            return string.Format("{0}{1}{2}", scope, ScopeSeparatorChar, path);
        }
    }
}
