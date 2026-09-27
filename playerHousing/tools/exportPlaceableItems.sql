-- Read-only export of every placeable item (the raw capture, before the item tail migration runs): first the columns that
-- identify an item's kind and model, then its raw housing tail in the column order tools/itemTailDecode.py reads.
SELECT id AS item_id, idfile, itemtype, icon, clicktype, clickeffect, scrolleffect, nodrop, price, lore FROM items WHERE UNK224 <> 0 ORDER BY id;
SELECT id, Name, UNK223, UNK224, UNK225, UNK226, UNK227, UNK228, UNK229, UNK230, UNK231, UNK232, UNK233, UNK234, placeable,
  UNK236, UNK237, UNK238, UNK239, UNK240, UNK241 FROM items WHERE UNK224 <> 0 ORDER BY id;
