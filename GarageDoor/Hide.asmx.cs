using System;
using GarageDoor.JsonObjects;
using Newtonsoft.Json;
using System.Diagnostics;
using System.Linq;
using System.Web;
using System.Web.Script.Services;
using System.Web.Services;

namespace GarageDoor
{
    /// <summary>
    /// PUBLIC WEBHOOK CALLS CAN GO HERE. But there is a difficult security problem to get around.
    /// No easy way secure a web method call using with ASP.Net forms authentication when the call 
    /// is being made by a Particle webhook. Web.config can be configured to exclude this *asmx from 
    /// needing Forms authentication but that means it's unsecured. These web methods are not easy to 
    /// discover but once they are known then nothing is restricting them from being called by anyone.
    /// </summary>
    [WebService(Namespace = "http://tempuri.org/")]
    [WebServiceBinding(ConformsTo = WsiProfiles.BasicProfile1_1)]
    [ScriptService]
    [System.ComponentModel.ToolboxItem(false)]
    public class Hide : System.Web.Services.WebService
    {
        /// <summary>
        /// Logs one sensor reading. Can be called by a webhook. 
        /// </summary>
        /// <param name="data">string containing json for reading of the sensors.</param>
        /// <returns>rows of data logged</returns>
        [WebMethod]
        public int PublishReportGargeStatus(string data)
        {
            try
            {
                var resp = JsonConvert.DeserializeObject<SensorReadingsForOperation>(data);
                Debug.WriteLine(data);
                return Log.LogSensor(resp);
            }
            catch (Exception ex)
            {
                Elmah.ErrorLog.GetDefault(HttpContext.Current).Log(new Elmah.Error(ex, HttpContext.Current));
                return -1;
            }
        }

        /// <summary>
        /// Receives notification from a particle publish with the of name a particle variable being
        /// passed as a parameter and logs that data.  Can be called by a Webhook.
        /// </summary>
        /// <param name="data">Name of particle variable holding data to be retrieved. Expecting this string
        /// matches an enum Is an enum string
        /// so different logic can be used the do whatever is needed for that particular type.</param>
        /// <returns>rows of data logged</returns>
        [WebMethod]
        //public int PublishDataReady(ParticleVariables data)
        public int PublishDataReady(ParticleVariables data)
        {
            try
            {
                // In this case the retrieved particle variable is being logged to DB
                if (data == ParticleVariables.moveReadings)
                {
                    var jStr = WebService1.DoGetVariable(data);
                    var jObj = JsonConvert.DeserializeObject<SensorReadingsForOperation>(jStr);

                    var readings = jObj.Readings.Select(r => $"t:{r.LogTime} r:{r.Rpm}");
                    Debug.WriteLine($"{string.Join("\n", readings.ToArray())}");

                    return Log.LogSensor(jObj);
                }
                return -1;

            }
            catch (Exception ex)
            {
                Elmah.ErrorLog.GetDefault(HttpContext.Current).Log(new Elmah.Error(ex, HttpContext.Current));
                return -1;
            }
        }
    }
}
