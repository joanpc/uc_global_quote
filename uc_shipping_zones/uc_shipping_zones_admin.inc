<?php
/**
 * Theme zones admin form
 */

function theme_uc_shipping_zones_admin($form){
  $header = array(t('Delete'), t('Name'), t('Countries'));
  $result = db_query("SELECT * FROM {uc_shipping_zones} ORDER BY name"); 
  
  while ($r = db_fetch_object($result)) {
    $row = array();
    $zid = $r->zid;
    $row[] = drupal_render($form['zones'][$zid]['delete']);
    $row[] = $r->name;
    $row[] = str_replace('|', ' ', $r->countries);
    $row[] = l(t('edit'), 'admin/store/settings/quotes/methods/zones/'. $zid);
    $rows[] = $row;
  }
    
  if (count($rows) == 0) {
    $rows[] = array(array('data' => t('No zones found'), 'colspan' => 4));
  }
  $output .= theme('table', $header, $rows);
  
  $output .= drupal_render($form);
  return $output;
}

/*
 * uc_shipping_zones admin form
 */

function uc_shipping_zones_admin($form_state, $id = 0){
  $form = array();
  $form['zones'] = array(
    '#tree' => true
  );
  
  if ($id) $field = "Edit Zone";
  else {
    $field = 'Add a new zone';
    $edit->name = NULL;
    $edit->id = NULL;
    $edit->country = NULL;
  }
  
  $zquery = db_query("SELECT * FROM {uc_shipping_zones}");
  while ($zone = db_fetch_object($zquery)) {
    $zid = $zone->zid;
    $form['zones'][$zid]['delete'] = array(
      '#type' => 'checkbox',
      '#default_value' => 0,
    );
    // Edit zone
    if ($id && $id == $zid) {
        $edit = $zone;
        $edit->countries = explode('|', $zone->countries);
    }
  }

  $countries = array();  
  foreach (uc_get_country_data() as $c) {
      $countries[$c['country_name'] . ',' . $c['country_id']] = $c['country_name'];
  } 
  
  $form['zone'] = array(
    '#type' => 'fieldset',
    '#title' => t($field),
    '#description' => t('Shipping zone options'),
    '#collapsible' => true,
    '#collapsed' => false,
  );
  
  $form['zone']['zid'] = array(
    '#type' => 'hidden',
    '#value' => $edit->zid,
   );
  
  $form['zone']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Zone name'),
    '#default_value' => '',
    '#default_value' => $edit->name,
    '#size' => 40,
    '#description' => t('Name for the zone'),
   );
  
  $form['zone']['country'] = array(
    '#type' => 'select',
    '#title' => t('Countries'),
    '#options' => $countries,
    '#default_value' => $edit->countries,
    '#multiple' => TRUE,
    '#description' => t('Select all the countries of the zone. ') . l('Add countries', 'admin/store/settings/countries/edit'),
   );

  $form['buttons']['submit'] = array('#type' => 'submit', '#value' => t('Submit Changes') );
  $form['#redirect'] = 'admin/store/settings/quotes/methods/zones';
  
  return $form;
}


function uc_shipping_zones_admin_validate($form, &$form_state){
      if (!is_array($form_state['values']['zones']) || !array_search(array('delete' => 1),$form_state['values']['zones'])){
      if (!$form_state['values']['name']) {
           form_set_error('name', t('Enter a name for the zone'));
      }
      if(!is_array($form_state['values']['country']) || count($form_state['values']['country']) < 1) {
         form_set_error('country', t('You must select a country for your zone'));
      }
      else {
          // Check overlapping zones.
          $zones = uc_shipping_zones_get();
          $cerror = array();
          foreach($zones as $zone) {
              if(!$form_state['values']['zid'] || $form_state['values']['zid'] != $zone->zid){
                  foreach($form_state['values']['country'] as $c) {
                      if (in_array($c, explode('|', $zone->countries))) $cerror[] = $c;
                }
              }
          }
          if (count($cerror)) form_set_error('country', t('The following countries are selected in other zones: %list', array('%list' => implode(', ', $cerror))));          
      }
    }
    
}

/*
 * Admin submit form
 */

function uc_shipping_zones_admin_submit($form, &$form_state){
  // check for and handle any deletions
  if(is_array($form_state['values']['zones'])) {
    foreach ($form_state['values']['zones'] as $zid => $value)
      if ($value['delete']) {
        db_query("DELETE FROM {uc_shipping_zones} WHERE zid = %d", $zid);
      }
  }
  
  if ($form_state['values']['name'] && is_array($form_state['values']['country']) && count($form_state['values']['country'])) {
      $countries = implode('|', $form_state['values']['country']);
      
      if ($form_state['values']['zid']) {
          db_query("UPDATE {uc_shipping_zones} SET name='%s', countries='%s' WHERE zid='%d'", check_plain($form_state['values']['name']), $countries, $form_state['values']['zid']);
      }
      else {
          db_query("INSERT INTO {uc_shipping_zones} (name, countries) VALUES ('%s','%s')", check_plain($form_state['values']['name']), $countries);
      }
  }
  
}