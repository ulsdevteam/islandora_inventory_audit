# upitt_islandora_inventory_audit

This is a stand-alone module that will report on the inventory of the same objects across legacy systems and compare to the current ingested objects in our Islandora systems.

It was easier and faster to just create a text file of the files that are on bigfoot and on equinox.  These files are created using the command `find /usr/local/dlsx/repository/ -name *.tif* -o -name *.xml > ~/inv_repo.out`.

TODO: add a PhotoDB component to the inventory counts - to be able to compare that system to our islandora.