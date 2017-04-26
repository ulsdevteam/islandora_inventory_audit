<?php print implode(" | " , $links); ?>
<h3><?php print $table; ?></h3>

<?php if($count_rows): ?>
  <?php print number_format($count_rows); ?> record<?php print ($count_rows == 1) ? '' : 's'; ?><br>
<?php endif; ?>

<?php print $pager; ?>
<div style="overflow-x:auto">
  <?php print $table_markup; ?>
</div>
