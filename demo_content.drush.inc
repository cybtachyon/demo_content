<?php

/**
 * @file
 * Drush functions for the demo_content module.
 */

use Drush\Log\LogLevel;

/**
 * Implements hook_drush_command().
 */
function demo_content_drush_command() {
  $items = array();
  $items['demo-content-import'] = array(
    'description' => 'Import demo content from a module or theme.',
    'aliases' => array('dci'),
    'arguments' => array(
      'file' => 'The name of the theme or module containing demo content.',
    ),
    'options' => array(
      'filter' => 'A filter for the demo content being imported.',
    ),
    'examples' => array(
      'demo-content-import mymodule --filter=node.article' =>
      'Import only the node.article demo file from the custom module mymodule.',
    ),
  );
  return $items;
}

/**
 * Drush command callback for importing demo content.
 *
 * @param string $module
 *   The name of the theme or module containing demo content.
 *
 * @TODO: Add support for other content importer plugins.
 */
function drush_demo_content_import($module) {
  $info = system_get_info('module', $module);
  $info += system_get_info('theme', $module);
  $demo_json = !empty($info['demo_content']);
  $demo_content = $demo_json ? $info['demo_content'] : array();
  $filter = drush_get_option('filter');
  foreach ($demo_content as $file_index => $demo_file) {
    if (!strpos($demo_file, '.json')) {
      $demo_json = FALSE;
    }
    if (!empty($filter) && !strpos($demo_file, $filter)) {
      unset($demo_content[$file_index]);
    }
  }
  if (!$demo_json) {
    drush_log(dt('No demo_content found in @module.info.',
      array('@module' => $module)), LogLevel::ERROR);
    return;
  }
  if (!module_exists('json_content')) {
    drush_log(dt('Module providing json file content import not found.'),
      LogLevel::ERROR);
    return;
  }
  $demo_path = DRUPAL_ROOT
    . DIRECTORY_SEPARATOR
    . drupal_get_path('module', $module);
  $ci = array();
  $ci_count = 0;
  foreach ($demo_content as $demo_file) {
    $result = FALSE;
    try {
      $result = json_content_import($demo_path, $demo_file);
    }
    catch (RuntimeException $exception) {
      $errors[] = $exception;
    }
    if (!is_array($result)) {
      continue;
    }
    foreach ($result as $type => $content) {
      $ids = array_keys($content);
      $ci[$type][$ids[0]] = $content;
      $ci_count++;
    }
  }
  if (empty($ci)) {
    $errors[] = new RuntimeException('No demo content found.');
  }
  if (!empty($errors)) {
    foreach ($errors as $error) {
      drush_log($error->getMessage(), LogLevel::WARNING);
    }
    drush_log(dt('Error importing demo_content from @module. See log for info.',
      array('@module' => $module)), LogLevel::ERROR);
    return;
  }
  $count = count($ci);
  $message = format_plural(
    $count,
    'Successfully imported one content type, @ci content from @module.',
    'Successfully imported @count content types, @ci content from @module.',
    array('@module' => $module, '@ci' => $ci_count));
  drush_log($message, LogLevel::OK);
}
