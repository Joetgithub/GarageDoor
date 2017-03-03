<%@ Page Language="C#" AutoEventWireup="true" CodeBehind="Logon.aspx.cs" Inherits="GarageDoor.Logon" %>

<%@ Import Namespace="System.Web.Security" %>

<script runat="server">
    void Logon_Click(object sender, EventArgs e)
    {
#if (!NOAUTH)
        if ((UserEmail.Text == ConfigurationManager.AppSettings["user"]) &&
                (UserPass.Text == ConfigurationManager.AppSettings["pw"]))
        {
            FormsAuthentication.RedirectFromLoginPage(UserEmail.Text, Persist.Checked);
        }
        else
        {
            Msg.Text = "Invalid credentials. Please try again.";
        }
#endif
    }
</script>

<!DOCTYPE html>
<html>
<head id="Head1" runat="server">
    <title>Logon</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">

    <link rel="stylesheet" href="Content/jquery.mobile-1.4.5.min.css">
    <script src="Scripts/jquery-1.9.1.min.js"></script>

    <%--http://stackoverflow.com/questions/14138014/page-url-is-not-changing-on-redirecting--%>
    <script>
        $(document).bind("mobileinit", function () {
            $.mobile.ajaxEnabled = false;
        });
    </script>

    <script src="Scripts/jquery.mobile-1.4.5.min.js"></script>
</head>
<body>
    <div data-role="page">
        <form id="form1" runat="server">
            <div data-role="main" class="ui-content">
                <table>
                    <tr>
                        <td>User:</td>
                        <td>
                            <asp:TextBox ID="UserEmail" runat="server" /></td>
                        <td>
                            <asp:RequiredFieldValidator ID="RequiredFieldValidator1"
                                ControlToValidate="UserEmail"
                                Display="Dynamic"
                                ErrorMessage="Cannot be empty."
                                runat="server" />
                        </td>
                    </tr>
                    <tr>
                        <td>Password:</td>
                        <td>
                            <asp:TextBox ID="UserPass" TextMode="Password" runat="server" />
                        </td>
                        <td>
                            <asp:RequiredFieldValidator ID="RequiredFieldValidator2"
                                ControlToValidate="UserPass"
                                ErrorMessage="Cannot be empty."
                                runat="server" />
                        </td>
                    </tr>
                    <tr>
                        <td colspan="3">
                            <asp:Button ID="Submit1" OnClick="Logon_Click" Text="Log On" runat="server" />
                        </td>
                    </tr>
                    <tr>
                        <td colspan="3">
                            <asp:CheckBox ID="Persist" runat="server" Text="Remember Me?" /></td>
                    </tr>
                    <tr>
                        <td colspan="3">
                            <asp:Label ID="Msg" ForeColor="red" runat="server" />
                        </td>
                    </tr>
                </table>
            </div>
        </form>
    </div>
</body>
</html>
