<?php
    include('config.php');

    $connection = new mysqli($host, $user, $password, $database);
    // Check connection
    if ($connection->connect_error) {
        die("Connection failed: " . $connection->connect_error);
    }

    if(isset($_POST["view_data"])){
        header('Location:tabular_data.php');
    }

?>
<!DOCTYPE html>
<html>
    <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ETX Telemetry Data</title>
    <style>
                /* Add CSS styles for the date range selection */
        body {
            font-family: Arial, sans-serif;
            text-align: left;
        }
        .date-range-container {
            margin: 50px auto;
        } 
        .date-range-container input[type="date"] {
            padding: 5px;
        }
        .date-range-container button {
            padding: 5px 10px;
            margin: 0 5px;
        }

        h2 {
        font-family: Arial, sans-serif;
        text-align: center;
        }
        table {
        border-collapse: collapse;
        width: 50%;
        margin: 10px auto;
        border:none;
        }
        th, td {
        border: 1px solid black;
        padding: 8px;
        border:none;
        }
        th {
        background-color: #f2f2f2;
        border:none;
        }
        tr{
        border-bottom: 1px solid #B2BEB5;
        }
        div {
        margin: 5px auto;
        text-align: center;
        }

        span {
        background-color:none;
        }
        .card {
        box-shadow: 0 4px 8px 0 rgba(0,0,0,0.2);
        padding: 10px;
        background-color: #f1f1f1;  
        }

        .card:hover {
        box-shadow: 0 8px 16px 0 rgba(0,0,0,0.2);
        }
        /* Float four columns side by side */
        .cardcolumn {
        float: left;
        width: 18%;
        padding: 0 10px;
        }

        .anglecardcolumn{
            float: left;
            width: 13%;
            padding: 0 10px;
        }

        /* Remove extra left and right margins, due to padding */
        .cardrow {margin: 0 auto;}

        /* Clear floats after the columns */
        .cardrow:after {
        content: "";
        display: table;
        clear: both;
        }


        /* Responsive columns */
        @media screen and (max-width: 900px) {
        .cardcolumn {
            width: 100%;
            display: block;
            margin-bottom: 20px;
        }
        }

        .cardnames td{
            padding:8px;
        }

        .cardnames tr {
            border: none;
            text-align: left;
        }
    </style>
   
    </head>
    <body>
    <div style="text-align: left;"class="date-range-container">
        <h2>ETX Telemetry Data</h2>
        <form id="myform"  method="get">
            <label for="etx_id">Choose ETX Id: </label>
            <!-- <select name="etx_id" id="etx_id" onchange="myform.submit();"> -->
            <select name="etx_id" id="etx_id" onchange="selectIDChanged()">
                <option>Select</option>
                <option value="VLX01">P08</option>
                <option value="VLX02">P07</option>
            </select>
        </form>
    </div>
        <script>
            function display_c(){
                var refresh=1000; // Refresh rate in milli seconds
                mytime=setTimeout('display_ct()',refresh)
            }

            function display_ct() {
                var x = new Date()
                var ampm = x.getHours( ) >= 12 ? ' PM' : ' AM';
                hours = x.getHours( ) % 12;
                hours = hours ? hours : 12;
                hours=hours.toString().length==1? 0+hours.toString() : hours;

                var minutes=x.getMinutes().toString()
                minutes=minutes.length==1 ? 0+minutes : minutes;

                var seconds=x.getSeconds().toString()
                seconds=seconds.length==1 ? 0+seconds : seconds;

                var month=(x.getMonth() +1).toString();
                month=month.length==1 ? 0+month : month;

                var dt=x.getDate().toString();
                dt=dt.length==1 ? 0+dt : dt;

                var x1=month + "/" + dt + "/" + x.getFullYear(); 
                x1 = x1 + " - " +  hours + ":" +  minutes + ":" +  seconds + " " + ampm;
                document.getElementById('ct').innerHTML = x1;
                display_c();
            }
           
            function loadXMLDoc(linkId,sourceUrl) {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                document.getElementById(linkId).innerHTML =
                this.responseText;
                }
            };
            xhttp.open("GET", sourceUrl, true);
            xhttp.send();
            }
            setInterval(function selectIDChanged() {
                var selectedID=document.getElementById("etx_id").value;
                loadXMLDoc("link1","column_cards.php?etx_id="+selectedID);
                loadXMLDoc("link2","angle_cards.php?etx_id="+selectedID);
            },1000)

        

            window.onload=loadXMLDoc;
            window.onload=display_ct;
        </script>

    <span  id='ct'>

    </span>
    
    <div id="link1" class="cardrow">
    
    </div>

    <div id="link2" class="cardrow">
    
    </div>
    <div style="text-align: left;"class="date-range-container">
        <form method="post" action="">
            <button type="submit" name="view_data" value="redirect">View All Data</button>
        </form>
    </div>
    
    </body>
</html>