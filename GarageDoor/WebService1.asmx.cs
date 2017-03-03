using GarageDoor.Particle;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq; //JObject
using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Web;
using System.Web.Script.Services;
using System.Web.Services;

namespace GarageDoor
{

    [WebService(Namespace = "http://tempuri.org/")]
    [WebServiceBinding(ConformsTo = WsiProfiles.BasicProfile1_1)]
    [ScriptService]
    [System.ComponentModel.ToolboxItem(false)]
    public class WebService1 : WebService
    {
        private const string JsonContentType = "application/json";

        /// <summary>
        /// Returns a Particle.variable. Requires that the incoming request be json content type.
        /// </summary>
        /// <param name="particleVariable">ParticleVariables enum type</param>
        /// <returns>particle variable string</returns>
        /// <exception cref="ApplicationException"></exception>
        [WebMethod]
        public string GetVariable(ParticleVariables particleVariable)
        {
            return DoGetVariable(particleVariable);
        }

        /// <summary>
        /// Executes the Particle function "DoorCommand".  We can only get back an integer from the photon for a function.
        /// </summary>
        /// <param name="operation">DoorCommands enum</param>
        /// <returns>Even though this returns a void it's actually returning a string via a Response.Write
        /// This is hack way to send back a JSON string without the XML wrappers added by default.  </returns>
        [WebMethod]
        public void DoorCommand(DoorCommands operation, int startStatus, DoorCommands lastCommand)
        {
            Stopwatch logSw = new Stopwatch();
            logSw.Start();

            try
            {
                WebRequest request = WebRequest.Create(EnvironmentVariables.ParticleUrl + "/DoorCommand");
                request.Method = "POST";
                request.ContentType = "application/x-www-form-urlencoded";

                if (!EnvironmentVariables.IsLocal)
                {
                    request.Proxy = new WebProxy("ntproxyus.lxa.perfora.net", 3128);
                }

                string paramString = $"params={operation}&access_token={EnvironmentVariables.AccessToken}";
                byte[] paramBytes = Encoding.UTF8.GetBytes(paramString);
                request.ContentLength = paramBytes.Length;

                JProperty return_value;
                using (Stream requestStream = request.GetRequestStream())
                {
                    requestStream.Write(paramBytes, 0, paramBytes.Length);  //...but response is not accepted till this executes
                    WebResponse response = request.GetResponse();
                    using (Stream responseStream = response.GetResponseStream())
                    {
                        StreamReader sr = new StreamReader(responseStream);
                        var results = sr.ReadToEnd();
                        JObject jObj = JObject.Parse(results);
                        return_value = jObj.Properties().FirstOrDefault(p => p.Name == "return_value");
                    }
                }

                logSw.Stop();
                Log.LogDoorCommand(lastCommand, startStatus, operation, (int)return_value.Value, logSw.ElapsedMilliseconds);

                HttpContext.Current.Response.Write($"{{{return_value}}}");
            }
            catch (Exception ex)
            {
                logSw.Stop();
                Elmah.ErrorLog.GetDefault(HttpContext.Current).Log(new Elmah.Error(ex, HttpContext.Current));
                // send junk back to the client
                var msg = $"DoorCommand Exception: {ex.Message} {ex.StackTrace}";
                HttpContext.Current.Response.Write($"{msg}");
            }
        }

        public static string DoGetVariable(ParticleVariables particleVariable)
        {
            Stopwatch logSw = new Stopwatch();
            logSw.Start();

            try
            {
                if (HttpContext.Current.Request.Headers["Content-Type"] != null &&
                    HttpContext.Current.Request.Headers["Content-Type"].IndexOf(JsonContentType, StringComparison.OrdinalIgnoreCase) == -1)
                {
                    throw new ApplicationException($"Must use Contenty-Type of '{JsonContentType}'");
                }

                var url = $"{EnvironmentVariables.ParticleUrl}/{particleVariable}?access_token={EnvironmentVariables.AccessToken}";
                HttpWebRequest request = (HttpWebRequest)WebRequest.Create(url);
                request.Method = "GET";

                // when running on 1and1 host you can't make a request except through their proxy
                if (!EnvironmentVariables.IsLocal)
                {
                    request.Proxy = new WebProxy("ntproxyus.lxa.perfora.net", 3128);
                }

                string respRaw;
                using (HttpWebResponse response = (HttpWebResponse)request.GetResponse())
                using (Stream strm = response.GetResponseStream())
                {
                    StreamReader strmRdr = new StreamReader(strm);
                    respRaw = strmRdr.ReadToEnd();
                }

                var particleVariableResponse = JsonConvert.DeserializeObject<ParticleVariableResponse>(respRaw);

                logSw.Stop();

                return $"{particleVariableResponse.result}";
            }
            catch (Exception ex)
            {
                logSw.Stop();
                Elmah.ErrorLog.GetDefault(HttpContext.Current).Log(new Elmah.Error(ex, HttpContext.Current));
                // send junk back to the client
                var logMsg = $"GetVariable Exception: {ex.Message} {ex.StackTrace}";
                return logMsg;
            }
            #region Pass Cookies experiment
            //HttpCookie x = Context.Request.Cookies[".ASPXFORMSAUTH"];
            //CookieContainer cc = new CookieContainer();
            //Uri uri = new Uri("http://local.garagedoor/");
            //cc.Add(new Cookie("xxxxxxx", x.Value) { Domain = uri.Host });
            //request.CookieContainer = cc;
            #endregion

        }
    }
}
