﻿//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version:2.0.50727.5466
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

namespace CreatorIDE.Engine {
    using System;
    
    
    /// <summary>
    ///   A strongly-typed resource class, for looking up localized strings, etc.
    /// </summary>
    // This class was auto-generated by the StronglyTypedResourceBuilder
    // class via a tool like ResGen or Visual Studio.
    // To add or remove a member, edit your .ResX file then rerun ResGen
    // with the /str option, or rebuild your VS project.
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("System.Resources.Tools.StronglyTypedResourceBuilder", "2.0.0.0")]
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
    [global::System.Runtime.CompilerServices.CompilerGeneratedAttribute()]
    internal class Resources {
        
        private static global::System.Resources.ResourceManager resourceMan;
        
        private static global::System.Globalization.CultureInfo resourceCulture;
        
        [global::System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
        internal Resources() {
        }
        
        /// <summary>
        ///   Returns the cached ResourceManager instance used by this class.
        /// </summary>
        [global::System.ComponentModel.EditorBrowsableAttribute(global::System.ComponentModel.EditorBrowsableState.Advanced)]
        internal static global::System.Resources.ResourceManager ResourceManager {
            get {
                if (object.ReferenceEquals(resourceMan, null)) {
                    global::System.Resources.ResourceManager temp = new global::System.Resources.ResourceManager("CreatorIDE.Engine.Resources", typeof(Resources).Assembly);
                    resourceMan = temp;
                }
                return resourceMan;
            }
        }
        
        /// <summary>
        ///   Overrides the current thread's CurrentUICulture property for all
        ///   resource lookups using this strongly typed resource class.
        /// </summary>
        [global::System.ComponentModel.EditorBrowsableAttribute(global::System.ComponentModel.EditorBrowsableState.Advanced)]
        internal static global::System.Globalization.CultureInfo Culture {
            get {
                return resourceCulture;
            }
            set {
                resourceCulture = value;
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to A category with the name &apos;{0}&apos; allready exists..
        /// </summary>
        internal static string CategoryWithNameExistsFormat {
            get {
                return ResourceManager.GetString("CategoryWithNameExistsFormat", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to An empty string is not a valid name for a category..
        /// </summary>
        internal static string EmptyCategoryNameDisallowed {
            get {
                return ResourceManager.GetString("EmptyCategoryNameDisallowed", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to An empty string is not a valid name for an entity..
        /// </summary>
        internal static string EmptyEntityNameDisallowed {
            get {
                return ResourceManager.GetString("EmptyEntityNameDisallowed", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Engine initialization has failed with code {0}..
        /// </summary>
        internal static string EngineInitFailFormat {
            get {
                return ResourceManager.GetString("EngineInitFailFormat", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to An entity with the name &apos;{0}&apos; allready exists..
        /// </summary>
        internal static string EntityWithNameExistsFormat {
            get {
                return ResourceManager.GetString("EntityWithNameExistsFormat", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Entity.
        /// </summary>
        internal static string NewEntityDefaultName {
            get {
                return ResourceManager.GetString("NewEntityDefaultName", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to String is too long. Maximal buffer size has been exceeded..
        /// </summary>
        internal static string StringBufferExceeded {
            get {
                return ResourceManager.GetString("StringBufferExceeded", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Value &apos;{0}&apos; is not supported..
        /// </summary>
        internal static string ValueNotSupportedFormat {
            get {
                return ResourceManager.GetString("ValueNotSupportedFormat", resourceCulture);
            }
        }
    }
}
