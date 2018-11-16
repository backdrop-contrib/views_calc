Views Calc
============

This module adds simple calculations to a views table.

You can SUM, AVERAGE, COUNT, MIN, MAX, or AVG selected columns. A plug in theme makes the view look more spreadsheet-like (right justifies numeric fields, shades calculated columns and rows, and underlines calculated rows).

You can also create custom dynamic fields that can be used in any view using SQL snippets like CONCAT('field_first_name', ' ', 'field_last_name') or DATE_FORMAT(node.created, '%m/%%d/%Y'). The custom fields are set up in admin/settings/views_calc.

Installation
------------

- Install this module using the official Backdrop CMS instructions at
  https://backdropcms.org/guide/modules

Issues
------

Bugs and Feature requests should be reported in the Issue Queue:
https://github.com/backdrop-contrib/views_calc/issues

Current Maintainers
-------------------

- Joseph Flatt (https://github.com/hosef)

Credits
-------

- Ported to Backdrop CMS by Joseph Flatt (https://github.com/hosef).
- Maintained for Drupal by Miro Dietiker (https://www.drupal.org/u/miro_dietiker).
- Maintained for Drupal by Karen Stevenson (https://www.drupal.org/u/karens).
- Initial Drupal development by Karen Stevenson (https://www.drupal.org/u/karens).

License
-------

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.
