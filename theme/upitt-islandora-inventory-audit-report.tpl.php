<?php if ($csv_download_link): ?>
  <a href="<?php print $csv_download_link; ?>">Download CSV</a><br />
<?php endif; ?>
<h3><?php print $title; ?></h3>
<?php if (is_array($year_links) && count($year_links > 0)): ?>
 <p>View data by the year: <?php print implode(" | ", $year_links); ?></p>
<?php endif; ?>

<?php if ($report_detail) { print $report_detail; } ?>

<?php if($count_rows): ?>
  <?php print number_format($count_rows); ?> record<?php print ($count_rows == 1) ? '' : 's'; ?><br>
<?php endif; ?>
<?php if($total_object_count): ?>
  <?php print number_format($total_object_count); ?> total objects<br>
<?php endif; ?>

<form action="<?php print $get_from_uri; ?>" method="get" id="inventory_audit_filtering" accept-charset="UTF-8">
<?php print $filter_controls; ?>
</form>

<?php print $pager; ?>
<div style="overflow-x:auto">
  <?php print $table_markup; ?>
</div>
