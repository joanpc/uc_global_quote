<?php

/**
 * Configures the store shipping rates
 */
function uc_global_quote_admin($form_state, $id = 0){
  $form = array();
  $form['rates'] = array(
    '#tree' => true
  );
  
  if ($id) $label = 'Edit quote';
  else {
      $label = 'Add a new quote';
      $edit->id = NULL;
      $edit->qid = NULL;
      $edit->min = 0;
      $edit->max = 0;
      $edit->rate = 0.00;
      $edit->type = NULL;
  }
  
  $result = db_query("SELECT * FROM {uc_global_quote}");
  
  while ($r = db_fetch_object($result)) {
    $qid = $r->qid;
    $form['rates'][$qid]['delete'] = array(
      '#type' => 'checkbox',
      '#default_value' => 0,
    );
    if ($id && $id == $qid) {
        $edit = $r;
    }
    
  }
  // Get zones
  $zones = uc_shipping_zones_get_select();  

  $form['quote'] = array(
    '#type' => 'fieldset',
    '#title' => t($label),
    '#description' => t('Defines a range of weights for the order total weight.'),
    '#collapsible' => true,
    '#collapsed' => false,
  );
  
  $form['quote']['qid'] = array(
    '#type' => 'hidden',
    '#value' => $edit->qid,
   );

  $form['quote']['uc_global_quote_minimum'] = array(
    '#type' => 'textfield',
    '#title' => t('Minimum weight'),
    '#default_value' => number_format($edit->min, 2,'.',''),
    '#size' => 12,
    '#description' => t('minimum order weight in %unit', array('%unit' => variable_get('uc_weight_unit', 'lb'))),
  );
  $form['quote']['uc_global_quote_maximum'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum weight'),
    '#default_value' => number_format($edit->max, 2,'.',''),
    '#size' => 12,
    '#description' => t('maximum order weight in %unit', array('%unit' => variable_get('uc_weight_unit', 'lb'))),
  );
  $form['quote']['uc_global_quote_rate'] = array(
    '#type' => 'textfield',
    '#title' => t('Shipping Rate'),
    '#default_value' => number_format($edit->rate, variable_get('uc_currency_prec', 2)),
    '#size' => 12,
    '#description' => t('Shipping price in %currency', array('%currency' => variable_get('uc_currency_code', 'USD'))),
  );
  $form['quote']['uc_global_quote_zone'] = array(
    '#type' => 'select',
    '#title' => t('Shipping zone'),
    '#options' => $zones,
    '#default_value' => $edit->zid,
    '#multiple' => FALSE,
    '#description' => t('Select a zone for this rate. ') . l(t('edit/add zones'), 'admin/store/settings/quotes/methods/zones'), 
  );

  $form['buttons']['submit'] = array('#type' => 'submit', '#value' => t('Submit Changes') );
  $form['#redirect'] = 'admin/store/settings/quotes/methods/global_quote';
  return $form;
}

/** 
 * Admin form validation
 */

function uc_global_quote_admin_validate($form, &$form_state){
  // check if everything is a number greater than or equal to 0
  if ($form_state['values']['uc_global_quote_minimum'] < 0 || !is_numeric($form_state['values']['uc_global_quote_minimum'])) {
    form_set_error('uc_global_quote_minimum', t('The minimum value must be a number not less than 0.'));
  }
  if ($form_state['values']['uc_global_quote_maximum'] < 0 || !is_numeric($form_state['values']['uc_global_quote_maximum'])) {
    form_set_error('uc_global_quote_maximum', t('The maximum value must be a number greater than 0.'));
  }
  if (($form_state['values']['uc_global_quote_rate'] < 0) ||
      (!is_numeric($form_state['values']['uc_global_quote_rate']))) {
    form_set_error('uc_global_quote_rate', t('Shipping rate must be a number not less than 0.'));
  }
  
  // check that max is bigger than min
  if ($form_state['values']['uc_global_quote_maximum'] && $form_state['values']['uc_global_quote_maximum'] <= $form_state['values']['uc_global_quote_minimum']) {
    form_set_error('uc_global_quote_maximum', t('The maximum value must be greater than the minumum value.'));
  }
  
  if (!$form_state['values']['uc_global_quote_zone']){
      form_set_error('uc_global_quote_minimum', t('You must select a zone.'));
  }
  
  if ($form_state['values']['uc_global_quote_maximum'] > 0) {
    //check to make sure the new settings dont overlap any existing ranges
   
    if ($form_state['values']['qid']) $result = db_query("SELECT * FROM {uc_global_quote} WHERE zid=%f AND qid <> $d ORDER BY min", $form_state['values']['uc_global_quote_zone'], $form_state['values']['qid']);
    else $result = db_query("SELECT * FROM {uc_global_quote} WHERE zid=%f ORDER BY min", $form_state['values']['uc_global_quote_zone']);
 
    while ($r = db_fetch_object($result)) {
      if ($form_state['values']['uc_global_quote_minimum'] < $r->max && $form_state['values']['uc_global_quote_minimum'] >= $r->min) {
        form_set_error('uc_global_quote_minimum', t('The minimum value conflicts with an existing Range.'));
      }
      if($form_state['values']['uc_global_quote_maximum'] <= $r->max && $form_state['values']['uc_global_quote_maximum'] > $r->min) {
        form_set_error('uc_global_quote_maximum', t('The maximum value conflicts with an existing Range.'));
      }
    }
  }
}

/**
 * Admin form submit
 */

function uc_global_quote_admin_submit($form, &$form_state){
  // check for and handle any deletions
  if(is_array($form_state['values']['rates'])) {
    foreach ($form_state['values']['rates'] as $qid => $value)
      if ($value['delete']) {
        db_query("DELETE FROM {uc_global_quote} WHERE qid = %d", $qid);
      }
  }

  // check for and insert new rate quote
  if ($form_state['values']['uc_global_quote_maximum'] > $form_state['values']['uc_global_quote_minimum'] && $form_state['values']['uc_global_quote_maximum'] > 0){
    
    if($form_state['values']['qid']) {
     db_query("UPDATE {uc_global_quote} SET zid='%d', min='%f', max='%f', rate='%f' WHERE qid='%d'", $form_state['values']['uc_global_quote_zone'] , $form_state['values']['uc_global_quote_minimum'], $form_state['values']['uc_global_quote_maximum'], $form_state['values']['uc_global_quote_rate'], $form_state['values']['qid']);    
     
    }
    else { 
      db_query("INSERT INTO {uc_global_quote} (zid, min, max, rate) VALUES (%f, %f, %f, %f)", $form_state['values']['uc_global_quote_zone'], $form_state['values']['uc_global_quote_minimum'], $form_state['values']['uc_global_quote_maximum'], $form_state['values']['uc_global_quote_rate']);
    }
  }

  drupal_set_message(t('The configuration options have been saved.'));
}

/**
 * Theme admin form
 */

function theme_uc_global_quote_admin($form){
  $header = array(t('Zone'), t('Minimum value'), t('Maximum value'), t('Shipping Rate'), t('Delete'));
  $result = db_query("SELECT qid,min,max,rate,uc_global_quote.zid,uc_shipping_zones.name as zone FROM {uc_global_quote} LEFT JOIN {uc_shipping_zones} ON uc_shipping_zones.zid = uc_global_quote.zid ORDER by zone, min");
  
 
  while ($r = db_fetch_object($result)) {
    $row = array();
    $qid = $r->qid;
    $row[] = ($r->zone)? $r->zone : '--unnassigned--';
    $row[] = number_format($r->min, 0, variable_get('uc_currency_dec', '.'), '');
    $row[] = number_format($r->max, 0, variable_get('uc_currency_dec', '.'), '');
    $row[] = ($r->rate > 0)? uc_price($r->rate, array()) : 'Free';
    $row[] = drupal_render($form['rates'][$qid]['delete']);
    $row[] = l(t('edit'), 'admin/store/settings/quotes/methods/global_quote/'. $qid);
    $rows[] = $row;
  }
    
  if (count($rows) == 0) {
    $rows[] = array(array('data' => t('No rates found.'), 'colspan' => 4));
  }
  else {
    $rows[] = array(array('data' => t('Shipping quotes based on: ').variable_get('uc_global_quote_type','weight'), 'colspan' => 4));
  }
  $output .= theme('table', $header, $rows);
  
  $output .= drupal_render($form);
  
  return $output;
}
