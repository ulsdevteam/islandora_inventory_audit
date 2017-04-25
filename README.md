# upitt_islandora_inventory_audit

This is a stand-alone module that will report on the inventory of the same objects across legacy systems and compare to the current ingested objects in our Islandora systems.

In addition to the inventory on the other locations, the islandora objects are also queried for their parent object as well as the file size of ALL of their datastreams' versions as well as a total for only their latest versions. 

TODO: add a PhotoDB component to the inventory counts - to be able to compare that system to our islandora.

## Creating bigfoot / equinox "list files"
For the bigfoot and equinox inventory procedures, it was easier and faster to just create a text file of the relevant files.  These files are created using the command `find . /usr/local/dlsx/repository/ -name *.tif* -o -name *.xml > ~/bigfoot_repo.out`.

## Configuration
After creating the "list files" for bigfoot and equinox, they must be configured under `/admin/islandora/inventory_audit`.  There is no default value for these two fields; if these are not set, the system will not be able to calculate any inventory from the related servers.

## Usage
Clicking on any of the [Inventory *] buttons will clear the associated table before recalculating.

From the `/admin/islandora/inventory_audit`, all of the audit tables can be populated by clicking on the [Populate ALL audit tables] button.  Each inventory table could be populated individually by clicking one of the [Inventory *] buttons.

Be patient because inventory population takes a long time.
