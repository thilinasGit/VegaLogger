<?php
    include('config.php');
    $connection = new mysqli($host, $user, $password, $database);
    // Check connection
    if ($connection->connect_error) {
        die("Connection failed: " . $connection->connect_error);
    }

    // Step 2: Determine the current page number and calculate the starting record
    $recordsPerPage = 10; // Set the number of records to display per page
    if (isset($_GET['page']) && is_numeric($_GET['page'])) {
    $currentPage = intval($_GET['page']);
    } else {
    $currentPage = 1;
    }

    // Step 3: Fetch data from the "log" table with pagination
    $sql_count = "SELECT COUNT(*) as totalRecords FROM log"; // Count total records
    $result = $connection->query($sql_count);
    if(!$result){
        die('Error: '.$conn->error);
    }
    $row = $result->fetch_assoc();
    $totalRecords = $row['totalRecords'];
    if($totalRecords> 50000){
        $del_old_data="DELETE FROM log ORDER BY timestamp DESC LIMIT 1";
        if(!$connection->query($del_old_data)){
            die('Error: '.$conn->error);
        }
    }
    $totalPages = ceil($totalRecords / $recordsPerPage);

    // Calculate the starting record for the current page
    $startRecord = ($currentPage - 1) * $recordsPerPage;
    $id='';
    if(isset($_GET['etx_id'])){
        $id=$_GET['etx_id'];        
    }
    //write query for all data
    $sql="SELECT * FROM log WHERE ID ='$id' ORDER BY timestamp DESC LIMIT $startRecord,$recordsPerPage";
    //make query &get result
    $result=mysqli_query($connection,$sql);

    //fetch the resulting rows as an array
    $data=mysqli_fetch_all($result,MYSQLI_ASSOC);

    if (isset($_GET['start_date']) && isset($_GET['end_date'])) {

        $start_timestamp = $_GET['start_date'];
        $end_timestamp = $_GET['end_date'];

        
        $all_data="SELECT * FROM log WHERE timestamp >= '$start_timestamp 00:00:00' AND timestamp <= '$end_timestamp 23:59:59' ORDER BY timestamp DESC";

        $all_data_result=mysqli_query($connection,$all_data) or die("Selection Error " . mysqli_error($connection));

        $filename="ETX_telemetry_data_log ".date('Y-m-d H:i:s',time()).".csv";

        // Set header content-type to CSV and filename
        header('Content-Type: text/csv');
        header('Content-Disposition: attachment; filename="' . $filename . '";');

        $fp = fopen('php://output', 'w');

        $header = array('device_Id', 'solar_current', 'solar_voltage','solar_energy','solar_power','solar_soc','solar_temperature','mppt_current','mppt_voltage','mppt_energy','mppt_power','mppt_soc','mppt_temperature','battery_current','battery_voltage','battery_energy','battery_power','battery_soc','battery_temperature','ax','ay','az','X','Y','Z','mx','my','mz','timestamp');

        //add headers
        fputcsv($fp,$header);

        while($row = mysqli_fetch_assoc($all_data_result)){
            fputcsv($fp, $row);
        }

        fclose($fp);
        exit();

    }
?>

<!DOCTYPE html>
<html>
    <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Display MQTT Data</title>
    <style>
        /* Add CSS styles for the date range selection */
        body {
                font-family: Arial, sans-serif;
                text-align: center;
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
        margin: 20px auto;
        }
        th, td {
        border: 1px solid black;
        padding: 8px;
        }
        th {
        background-color: #f2f2f2;
        }
        div {
        margin: 20px auto;
        text-align: center;
        }
        .pagination-container {
        text-align: center;
        }
        .pagination {
        display: inline-block;
        }
        .pagination a {
        color: black;
        padding: 5px 10px;
        text-decoration: none;
        background-color: #f2f2f2;
        border: 1px solid #ddd;
        border-radius: 5px;
        }
        .pagination a.active {
        background-color: #4CAF50;
        color: white;
        }
        .pagination a:hover:not(.active) {
        background-color: #ddd;
        }
        span {
        background-color: #ccc;
        }
        .card {
        box-shadow: 0 4px 8px 0 rgba(0,0,0,0.2);
        transition: 0.3s;
        width: 40%;
        border-radius: 5px;
        }

        .card:hover {
        box-shadow: 0 8px 16px 0 rgba(0,0,0,0.2);
        }

        img {
        border-radius: 5px 5px 0 0;
        }

        .container {
        padding: 2px 16px;
        }
    </style>
    </head>
    <body>
    <div style="text-align: left;"class="date-range-container">
        <h2>MQTT Data Log and download</h2>
        <form id="myform"  method="get" action="">
            <label for="etx_id">Choose ETX Id: </label>
            <select name="etx_id" id="etx_id" onchange="myform.submit();">
                <option>Select</option>
                <option value="VLX01">P08</option>
                <option value="VLX02">P07</option>
            </select>
        </form>
    </div>
    <table>
        <tr>
        <th>ID</th>
        <th>A1</th>
        <th>V1</th>
        <th>E1</th>
        <th>P1</th>
        <th>S1</th>
        <th>T1</th>
        <th>A2</th>
        <th>V2</th>
        <th>E2</th>
        <th>P2</th>
        <th>S2</th>
        <th>T2</th>
        <th>A3</th>
        <th>V3</th>
        <th>E3</th>
        <th>P3</th>
        <th>S3</th>
        <th>T3</th>
        <th>ax</th>
        <th>ay</th>
        <th>az</th>
        <th>X</th>
        <th>Y</th>
        <th>Z</th>
        <th>mx</th>
        <th>my</th>
        <th>mz</th>
        <th>Timestamp</th>
        </tr>

    <?php
        // Step 4: Loop through the data and display it in the HTML table
        if ($result->num_rows> 0) {
            foreach ($result as $row) {
                echo "<tr>";
                echo "<td>" . $row['ID'] . "</td>";
                echo "<td>" . $row['A1'] . "</td>";
                echo "<td>" . $row['V1'] . "</td>";
                echo "<td>" . $row['E1'] . "</td>";
                echo "<td>" . $row['P1'] . "</td>";
                echo "<td>" . $row['S1'] . "</td>";
                echo "<td>" . $row['T1'] . "</td>";
                echo "<td>" . $row['A2'] . "</td>";
                echo "<td>" . $row['V2'] . "</td>";
                echo "<td>" . $row['E2'] . "</td>";
                echo "<td>" . $row['P2'] . "</td>";
                echo "<td>" . $row['S2'] . "</td>";
                echo "<td>" . $row['T2'] . "</td>";
                echo "<td>" . $row['A3'] . "</td>";
                echo "<td>" . $row['V3'] . "</td>";
                echo "<td>" . $row['E3'] . "</td>";
                echo "<td>" . $row['P3'] . "</td>";
                echo "<td>" . $row['S3'] . "</td>";
                echo "<td>" . $row['T3'] . "</td>";
                echo "<td>" . $row['ax'] . "</td>";
                echo "<td>" . $row['ay'] . "</td>";
                echo "<td>" . $row['az'] . "</td>";
                echo "<td>" . $row['X'] . "</td>";
                echo "<td>" . $row['Y'] . "</td>";
                echo "<td>" . $row['Z'] . "</td>";
                echo "<td>" . $row['mx'] . "</td>";
                echo "<td>" . $row['my'] . "</td>";
                echo "<td>" . $row['mz'] . "</td>";
                echo "<td>" . date('Y-m-d H:i:s', strtotime($row['timestamp'])) . "</td>";
                echo "</tr>";
            }
        } else {
        echo "<tr><td colspan='4'>No data available</td></tr>";
        }
        ?>
    </table>
    <div class="pagination-container">
        <div class="pagination">
            <?php
            // Step 5: Add pagination links
            for ($i = 1; $i <= $totalPages; $i++) {
            if ($i === $currentPage) {
                echo "<a class='active'>$i</a>"; // Display the current page number without a link
            } else {
                echo "<a href='tabular_data.php?page=$i&etx_id=$id'>$i</a>"; // Display other page numbers as links
            }
            }
            ?>
        </div>
    </div>
    <div style="text-align: left;"class="date-range-container">
        <form method="get" action="">
            From: <input type="date" name="start_date" value="<?php echo date('Y-m-d'); ?>" required>
            To: <input type="date" name="end_date" value="<?php echo date('Y-m-d'); ?>" required>
            <button type="submit">Download CSV</button>
        </form>
    </div>
    </body>
</html>