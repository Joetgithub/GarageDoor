using System;
using System.IO;
using System.Web;

namespace GarageDoor
{
    public class Global : System.Web.HttpApplication
    {

        protected void Application_Start(object sender, EventArgs e)
        {
        }

        protected void Session_Start(object sender, EventArgs e)
        {
        }

        protected void Application_BeginRequest(object sender, EventArgs e)
        {
            //var logFile = $"{Server.MapPath("~")}\\logs\\{DateTime.Now.ToString("yyyy.MMdd.HH")}.txt";
            //var r = ((HttpApplication) sender).Request;
            //var msg =
            //    $"\n\n{DateTime.Now.ToString("yyyy.MMdd.HHmm.ss.fff")}\nAppRelativeCurrentExecutionFilePath={r.AppRelativeCurrentExecutionFilePath}\nContentType={r.ContentType}\nCurrentExecutionFilePath={r.CurrentExecutionFilePath}\n" +
            //    $"Headers={r.Headers}\nIsAuthenticated={r.IsAuthenticated}\nIsLocal={r.IsLocal}\nIsSecureConnection={r.IsSecureConnection}\nPath={r.Path}\nPhysicalPath={r.PhysicalPath}\n" +
            //    $"QueryString={r.QueryString}\nRequestType={r.RequestType}\nUrl={r.Url}\nUserAgent={r.UserAgent}\nUserHostAddress={r.UserHostAddress}\nUserHostName={r.UserHostName}";
            //File.AppendAllText(logFile, msg);
#if (!NOAUTH)
            // 1) IsSecureConnection = does HTTP connection uses secure sockets (that is, HTTPS)?
            // 2) IsLocal = IP address of the request originator is 127.0.0.1 or
            //              IP address of the request is the same as the server's IP address
            if (HttpContext.Current.Request.IsSecureConnection.Equals(false) &&
                HttpContext.Current.Request.IsLocal.Equals(false))
            {
                Response.Redirect("https://" + Request.ServerVariables["HTTP_HOST"] + HttpContext.Current.Request.RawUrl);
            }
#endif
        }

        protected void Application_AuthenticateRequest(object sender, EventArgs e)
        {
        }

        protected void Application_Error(object sender, EventArgs e)
        {
            Exception ex = Server.GetLastError();
            Elmah.ErrorLog.GetDefault(HttpContext.Current).Log(new Elmah.Error(ex, HttpContext.Current));
            Response.Write("<h2>Global.asax Error has been logged.</h2>");
            Server.ClearError();
        }

        protected void Session_End(object sender, EventArgs e)
        {
        }

        protected void Application_End(object sender, EventArgs e)
        {
        }
    }
}