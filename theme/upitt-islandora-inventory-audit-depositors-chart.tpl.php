<div id="piechart" style="width: 1100px; height: 580px;"></div>
    <script type="text/javascript">
      google.charts.load('current', {'packages':['corechart']});
      google.charts.setOnLoadCallback(drawChart);

      function drawChart() {

        var data = google.visualization.arrayToDataTable([
          ['Depositor', 'Total Size'],
<?php $last_depositor = is_array($depositor_rows) && isset($depositor_rows[count($depositor_rows)]) ? $depositor_rows[count($depositor_rows)] : ''; ?>

<?php foreach ($depositors_rows as $k => $row): ?>
          [<?php print "'" . $row['depositor'] . "', " . $row['total']; ?>]<?php print ($row['depositor'] <> $last_depositor) ? ',' : ''; ?>
<?php endforeach; ?>
        ]);

        var options = {
          title: 'Depositor Size - (ignoring "University of Pittsburgh" content)',
          fontSize: 11,
          is3D: true,
        };

        var chart = new google.visualization.PieChart(document.getElementById('piechart'));

        chart.draw(data, options);
      }
    </script>

