using System;
using System.Configuration;
using System.Web;

namespace GarageDoor
{
    public static class EnvironmentVariables
    {
        public static string ServiceUrl => ConfigurationManager.AppSettings["serviceUrl"];

        public static string DeviceId => ConfigurationManager.AppSettings[ConfigurationManager.AppSettings["activePhoton"]];

        public static string AccessToken => ConfigurationManager.AppSettings["accessToken"];

        public static string ParticleUrl => ConfigurationManager.AppSettings["particleUrl"] + DeviceId;

        public static bool SqlLogging => Convert.ToBoolean(ConfigurationManager.AppSettings["sqlLogging"]);

        public static string ConnString => ConfigurationManager.ConnectionStrings["GarageDoor"].ConnectionString;

        public static string JoeLan => ConfigurationManager.AppSettings["joeLan"];

        public static string JoeWan => ConfigurationManager.AppSettings["joeWan"];

        public static bool IsLocal => HttpContext.Current.Request.IsLocal ||
                                      HttpContext.Current.Request.ServerVariables["HTTP_HOST"].StartsWith(JoeLan) ||
                                      HttpContext.Current.Request.ServerVariables["HTTP_HOST"].StartsWith(JoeWan);
    }
}