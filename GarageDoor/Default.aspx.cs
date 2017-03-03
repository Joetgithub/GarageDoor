using System;

namespace GarageDoor
{
    public partial class Default : System.Web.UI.Page
    {
        protected void Page_Load(object sender, EventArgs e)
        {
            //Welcome.Text = "Hello, " + Context.User.Identity.Name;
        }

        void Signout_Click(object sender, EventArgs e)
        {
            //FormsAuthentication.SignOut();
            //Response.Redirect("Logon.aspx");
        }
    }
}