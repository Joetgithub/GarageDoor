<%@ Page Language="C#" AutoEventWireup="true" CodeBehind="Default.aspx.cs" Inherits="GarageDoor.Default" %>

<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Garage Door</title>
    <link rel="stylesheet" href="Content/jquery.mobile-1.4.5.min.css">
    <script src="Scripts/jquery-1.9.1.min.js"></script>
    <script>
        $(document).bind("mobileinit", function () {
            $.mobile.ajaxEnabled = false;
        });

        $(document).ready(function () {
            getStatus();
        });
    </script>
    <script src="Scripts/jquery.mobile-1.4.5.min.js"></script>
    <script>
        var Status = {
            // sensor bits 1-8        //                 0000 0000
            Moving: 1,                //0                     0000
            DoorOpened: 2,            //1                     0010
            DoorClosed: 4,            //2                     0100
            Opening: 8,               //3                     1000
            Closing: 16,              //4                   1 0000
            // error bits 9-16        //       0000 0000 
            ErrGlobalState: 256,      //8              1 0000 0000
            ErrLastOperation: 512,    //9             10 0000 0000
            ErrMoveTimeout: 1024,     //10           100 0000 0000
            ErrCannotStopMove: 2048,  //11          1000 0000 0000
            ErrMoveNotStart: 4096,    //12        1 0000 0000 0000
            ErrWrongDirection: 8192,  //13       10 0000 0000 0000
        }

        var DoorCommands = {
            none: 0,
            pressButton: 1,
            awaitMoveComplete: 2,
            getStatus: 3,
        }

        <%= String.Format("var url = \"{0}\"", GarageDoor.EnvironmentVariables.ServiceUrl)  %>;

        var WEBMETH_DOORCOMMAND = 'DoorCommand';
        var WEBMETH_GETVAR = 'GetVariable';
        var WEBMETH_GETVAR_P1 = 'particleVariable';
        var lastStatus = 0;
        var lastCommand = DoorCommands.none;    

        function pressButton() {
            $('#errorMessage').css('display', 'none');
            $('#idPress').fadeTo(250, 0.5);
            $.ajax(
            {
                type: "POST",
                url: url + "/" + WEBMETH_DOORCOMMAND,
                data: { operation: "pressButton", startStatus: "" + lastStatus + "", lastCommand: "" + lastCommand + ""},
                success: function (result) {
                    try {
                        // if status is moving then send another async request which returns updated 
                        // status when movement (or timeout) completes
                        obj = JSON.parse(result);
                        lastStatus = obj.return_value;
                        lastCommand = DoorCommands.pressButton;
                        $('#idPress').fadeTo(50, 1);
                        displayCommandResponse($('#doorStatus'), DoorCommands.pressButton, lastStatus);

                        if (obj.return_value & Status.Moving) {
                            awaitMoveComplete();
                        }
                    }
                    catch (ex) {
                        alert(ex.message);
                    }
                },
                error: function (result) {
                    alert('ajax failure pressButton()' + '\n' + result.statusText);
                    $("#errorMessage").text("</br>Error!</br>Response:</br>" + result.responseText);
                    $("#errorMessage").css("display", "block");
                }
            });
        }
        function awaitMoveComplete() {
            $.ajax(
            {
                type: "POST",
                url: url + "/" + WEBMETH_DOORCOMMAND,
                data: { operation: "awaitMoveComplete", startStatus: "" + lastStatus + "", lastCommand: "" + lastCommand + ""},
                success: function (result) {
                    try {
                        obj = JSON.parse(result);
                        lastStatus = obj.return_value;
                        lastCommand = DoorCommands.awaitMoveComplete;
                        displayCommandResponse($("#doorStatus"), DoorCommands.awaitMoveComplete, lastStatus);
                    }
                    catch (ex) {
                        alert(ex.message);
                    }
                },
                error: function (result) {
                    alert('ajax failure awaitMoveComplete()' + '\n' + result.statusText);
                    $("#errorMessage").text("</br>Error!</br>Response:</br>" + result.responseText);
                    $("#errorMessage").css("display", "block");
                }
            });
        }
        //getStatus() seems like it should be implemented as a particle variable and not a command. 
        //But we need to execute logic on the photon when retrieving the status to determine if error conditions 
        //exist. It is not possible to do any logic on the photon when retrieving a particle variable. Note that  
        //the Photon has logic to clear the error bits whenever the command results are sent.
        function getStatus() {
            $('#errorMessage').css('display', 'none');
            if (lastCommand !== DoorCommands.none) { $('#doorStatus').fadeTo(250, 0.5); }
            $.ajax(
            {
                type: "POST",
                url: url + "/" + WEBMETH_DOORCOMMAND,
                data: { operation: "getStatus", startStatus: "" + lastStatus + "", lastCommand: "" + lastCommand + ""},
                success: function (result) {
                    try {
                        obj = JSON.parse(result);
                        lastStatus = obj.return_value;
                        lastCommand = DoorCommands.getStatus;
                        if (lastCommand !== DoorCommands.none) {$('#doorStatus').fadeTo(50, 1);}
                        displayCommandResponse($("#doorStatus"), DoorCommands.getStatus, lastStatus);
                    }
                    catch (ex) {
                        alert(ex.message);
                    }
                },
                error: function (result) {
                    alert('ajax failure getStatus()' + '\n' + result.statusText);
                    $("#errorMessage").text("</br>Error!</br>Response:</br>" + result.responseText);
                    $("#errorMessage").css("display", "block");
                }
            });
        }

        function displayCommandResponse(element, command, status) {
            var isErrorCondition =
                (status & Status.ErrGlobalState) ||
                (status & Status.ErrLastOperation) ||
                (status & Status.ErrMoveTimeout) ||
                (status & Status.ErrWrongDirection) ||
                (status & Status.ErrMoveNotStart);

            if (isErrorCondition) {
                $("#errorMessage").text("");
                $("#errorMessage").append("Error code:" + status + "</br>");
            }

            if (status & Status.ErrGlobalState) {
                $("#errorMessage").append("Global error Condition exits.</br>");
                $("#errorMessage").css("display", "block");
            }
            if (status & Status.ErrLastOperation) {
                $("#errorMessage").append("Last operation error condition exists.</br>");
                $("#errorMessage").css("display", "block");
            }
            if (status & Status.ErrMoveTimeout) {
                $("#errorMessage").append("Door is moving for too long.</br>");
                $("#errorMessage").css("display", "block");
            }
            if (status & Status.ErrWrongDirection) {
                $("#errorMessage").append("Door was moving UP and went to being CLOSED or was moving DOWN and went to OPEN.</br>");
                $("#errorMessage").css("display", "block");
            }
            if (command === DoorCommands.pressButton && status & Status.ErrMoveNotStart) {
                $("#errorMessage").append("Door failed to start moving.</br>");
                $("#errorMessage").css("display", "block");
            }
            if (command === DoorCommands.pressButton && status & Status.ErrCannotStopMove) {
                $("#errorMessage").append("Cannot stop door from moving.</br>");
                $("#errorMessage").css("display", "block");
                element.css("background-color", "#ff6666");
                element.css("color", "white");
            }

            // note this is a one big 'else .. else if' block
            if (status & Status.DoorOpened && !(status & Status.DoorClosed) && !(status & Status.Moving) && !isErrorCondition) {
                element.css("background-color", "green");
                element.css("color", "white");
                element.text("Open");
            }
            else if (status & Status.DoorClosed && !(status & Status.DoorOpened) && !(status & Status.Moving) && !isErrorCondition) {
                element.css("background-color", "#ff6666");
                element.css("color", "white");
                element.text("Closed");
            }
            else if (status & Status.Moving && status & Status.Opening && !isErrorCondition) {
                element.css("background-color", "#ff99ff");
                element.css("color", "white");
                element.text("Moving Up");
            }
            else if (status & Status.Moving && status & Status.Closing && !isErrorCondition) {
                element.css("background-color", "#ff99ff");
                element.css("color", "white");
                element.text("Moving Down");
            }
            else if (!(status & Status.DoorClosed) && !(status & Status.DoorOpened) && !(status & Status.Moving)) {
                element.css("background-color", "orange");
                element.css("color", "black");
                element.text("Door is partially opened");
            }
            else {
                element.css("background-color", "orange");
                element.css("color", "black");
                element.text(status + ": " + displayResults(status));
            }
        }

        function displayResults(theValue) {
            //var theValue =  $("#input").val();
            var results = "";
            if (theValue & Status.Moving) {
                results = results + ' Moving';
            }
            if (theValue & Status.DoorOpened) {
                results = results + ' DoorOpened' ;
            }
            if (theValue & Status.DoorClosed) {
                results = results + ' DoorClosed';
            }
            if (theValue & Status.Opening) {
                results = results + ' Opening';
            }
            if (theValue & Status.Closing) {
                results = results + ' Closing';
            }
            if (theValue & Status.ErrGlobalState) {
                results = results + ' ErrGlobalState';
            }
            if (theValue & Status.ErrLastOperation) {
                results = results + ' ErrLastOperation';
            }
            if (theValue & Status.ErrMoveTimeout) {
                results = results + ' ErrMoveTimeout';
            }
            if (theValue & Status.ErrCannotStopMove) {
                results = results + ' ErrCannotStopMove';
            }
            if (theValue & Status.ErrMoveNotStart) {
                results = results + ' ErrMoveNotStart';
            }
            if (theValue & Status.ErrWrongDirection) {
                results = results + ' ErrWrongDirection';
            }
            return results.trim();
        }

    </script>

</head>

<script runat="server">
    void Signout_Click(object sender, EventArgs e)
    {
        FormsAuthentication.SignOut();
        Response.Redirect("Logon.aspx");
    }
</script>

<body>
    <div data-role="page" >
        <div data-role="main" class="ui-content">
            <form id="Form1" runat="server">
                <asp:Button ID="Submit1" OnClick="Signout_Click" Text="Sign Out" runat="server" />
            </form>
            <button id="doorStatus" onclick="getStatus()">&nbsp;</button>
            <button id="idPress" onclick="pressButton()">Press</button>
            <div id="errorMessage" style="display: none"></div>
        </div>
    </div>
</body>
</html>
