<?php
/**
 * Theme zones admin form
 */

function theme_uc_shipping_zones_admin($form){
  $header = array(t('Delete'), t('Name'), t('Countries'), t('Regions'));
  $result = db_query("SELECT * FROM {uc_shipping_zones} ORDER BY name"); 
  
  while ($r = db_fetch_object($result)) {
    $row = array();
    $zid = $r->zid;
    $row[] = drupal_render($form['zones'][$zid]['delete']);
    $row[] = $r->name;
    $row[] = str_replace('|', ' ', $r->countries);
    if (count($r->regions)) $row[] = str_replace('|', ' ', $r->regions);
    else $row[] = 'All';
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
  
  ahah_helper_register($form, $form_state);
      
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
    $edit->regions = NULL;
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
        $edit->regions   = explode('|', $zone->regions);
    }
  }

  $countries = array();  
  foreach (uc_get_country_data() as $c) {
      $countries[$c['country_name'] . ',' . $c['country_id']] = $c['country_name'];
  } 
  
 if (isset($form_state['storage']['zone']['country']) ) {
    $edit->countries = $form_state['storage']['zone']['country'];
    unset($form_state['storage']['zone']['country']);
  }
  
  $form['zone'] = array(
    '#type' => 'fieldset',
    '#title' => t($field),
    '#description' => t('Shipping zone options'),
    '#collapsible' => true,
    '#collapsed' => false,
    '#prefix' => '<div id="shipping-zone-wrapper">', // This is our wrapper div.
    '#suffix' => '</div>',
    '#tree'   => TRUE, // Don't forget to set #tree!
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
    '#ahah' => array(
     'event' => 'change',
     'path' => ahah_helper_path(array('zone][region')), 
     'wrapper' => 'dropdown-region-replace',
    ),
   
   
   );
   
   $form['zone']['region'] = array(
    '#type' => 'select',
    '#title' => t('Region'),
    '#options' => uc_shipping_zones_regions_ajax($edit->countries),
    '#default_value' => $edit->regions,
    '#multiple' => TRUE,
    '#description' => t('Select regions. '),
    '#prefix' => '<div id="dropdown-region-replace">', 
    '#suffix' => '</div>',
   );

  $form['buttons']['submit'] = array('#type' => 'submit', '#value' => t('Submit Changes') );
  $form['#redirect'] = 'admin/store/settings/quotes/methods/zones';
  
  return $form;
}

function uc_shipping_zones_admin_validate($form, &$form_state) {
    if(!is_array($form_state['values']['zones']) || !array_search( array('delete' => 1), $form_state['values']['zones'])) {
        if(!$form_state['values']['zone']['name']) {
            form_set_error('name',  t('Enter a name for the zone'));
        }
        if(!is_array($form_state['values']['zone']['country']) || count($form_state['values']['zone']['country']) < 1) {
            form_set_error('country',  t('You must select a country for your zone'));
        } else {
            // Check overlapping zones.
            $zones = uc_shipping_zones_get();
            $cerror = array();
            foreach($zones as $zone) {
                if(!$form_state['values']['zone']['zid'] || $form_state['values']['zone']['zid'] != $zone->zid) {
                    foreach($form_state['values']['zone']['country'] as $c) {
                        if(in_array($c,  explode('|', $zone->countries)))
                            if ($zone->regions == NULL || array_key_exists('', $form_state['values']['zone']['region'])){
                               $cerror[] = $c;
                                
                            } else {
                                // Check overlapping regions
                                foreach($form_state['values']['zone']['region'] as $r) {
                                    if(in_array($r,  explode('|', $zone->regions))) $rerror[] = $r;
                                }
                            }
                    }
                }
            }
            if(count($cerror))
                form_set_error('country',  t('The following countries are selected in other zones: %list', array('%list' =>  implode(', ', $cerror))));
            if(count($rerror))
                form_set_error('region',  t('The following regions are selected in other zones: %list', array('%list' =>  implode(', ', $rerror))));
        }
    }
}

function uc_shipping_zones_regions_ajax($selection) {
  
  if (!count($selection)) return array('' => 'All');
  
  $countries = array();
  
  foreach ($selection as $country) {
    $cid = explode(',', $country);
    $countries[] = $cid[1]; 
  }
  $where = '';
  
  for ($i = 1; $i < count($countries); $i++ ){
    $where .= ' OR zone_country_id = %d ';
  }
   
  $result = db_query("SELECT * FROM {uc_zones} WHERE zone_country_id = %d" . $where . " ORDER BY 'zone_name'", $countries);
  
  $options[''] = t('All');
  while ($zone = db_fetch_object($result)) {
    $options[ $zone->zone_name . ',' .$zone->zone_id] = $zone->zone_name;
  }
  return $options;
}

/*
 * Admin form submit
 */

function uc_shipping_zones_admin_submit($form, &$form_state){
  // check for and handle any deletions
  if(is_array($form_state['values']['zones'])) {
    foreach ($form_state['values']['zones'] as $zid => $value)
      if ($value['delete']) {
        db_query("DELETE FROM {uc_shipping_zones} WHERE zid = %d", $zid);
      }
  }
  
  if ($form_state['values']['zone']['name'] && is_array($form_state['values']['zone']['country']) && count($form_state['values']['zone']['country'])) {
      $countries = implode('|', $form_state['values']['zone']['country']);
      $regions = implode('|', $form_state['values']['zone']['region']);
      
      if ($form_state['values']['zone']['zid']) {
          db_query("UPDATE {uc_shipping_zones} SET name='%s', countries='%s', regions='%s' WHERE zid='%d'", check_plain($form_state['values']['zone']['name']), $countries, $regions, $form_state['values']['zone']['zid']);
      }
      else {
          db_query("INSERT INTO {uc_shipping_zones} (name, countries, regions) VALUES ('%s','%s', '%s')", check_plain($form_state['values']['zone']['name']), $countries, $regions);
      }
  }
  
}