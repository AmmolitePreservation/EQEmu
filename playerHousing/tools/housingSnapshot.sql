-- The rows a housing request can change, for a diff before and after a run (stage 9's forged requests): every table a
-- handler writes, the items real estate holds, and the inventory and coin of every character who owns property.
-- mysql -N -B peq < housingSnapshot.sql > before.tsv
SELECT 'properties', p.* FROM real_estate_properties p ORDER BY p.id;
SELECT 'balances', b.* FROM real_estate_property_balances b ORDER BY b.property_id, b.currency;
SELECT 'items', r.* FROM real_estate_items r ORDER BY r.id;
SELECT 'accessGroups', g.* FROM real_estate_access_groups g ORDER BY g.property_id, g.group_number;
SELECT 'accessNames', n.* FROM real_estate_access_names n ORDER BY n.property_id, n.name;
SELECT 'polls', p.* FROM real_estate_polls p ORDER BY p.id;
SELECT 'pollOptions', o.* FROM real_estate_poll_options o ORDER BY o.poll_id, o.option_key;
SELECT 'pollVotes', v.* FROM real_estate_poll_votes v ORDER BY v.poll_id, v.account_id, v.option_key;
SELECT 'instances', i.* FROM real_estate_instances i ORDER BY i.instance_id;
SELECT 'realEstateItems', i.* FROM item_instances i WHERE i.holder_type = 'real_estate'
  ORDER BY i.guid_first, i.guid_second, i.guid_third;
SELECT 'ownerItems', i.* FROM item_instances i
  WHERE i.holder_type IN ('inventory', 'cursor', 'bank', 'parcel', 'trade') AND i.holder_id IN (SELECT owner_character_id FROM real_estate_properties WHERE owner_character_id <> 0)
  ORDER BY i.guid_first, i.guid_second, i.guid_third;
SELECT 'ownerCoin', c.* FROM character_currency c
  WHERE c.id IN (SELECT owner_character_id FROM real_estate_properties WHERE owner_character_id <> 0) ORDER BY c.id;
SELECT 'ownerAltCurrency', a.* FROM character_alt_currency a
  WHERE a.char_id IN (SELECT owner_character_id FROM real_estate_properties WHERE owner_character_id <> 0) ORDER BY a.char_id, a.currency_id;
