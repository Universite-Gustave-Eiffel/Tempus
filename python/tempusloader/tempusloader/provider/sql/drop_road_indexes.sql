drop function if exists _drop_index(text, text, text);
create function _drop_index(schema_name text, table_name text, col text) returns void
as
$$
declare
  idx_name text;
begin
  for idx_name in select relname
          from
            pg_index as idx
          join
            pg_class as i
          on
            i.oid = idx.indexrelid
          join pg_am as am
          on
            i.relam = am.oid
          where
            idx.indrelid::regclass = (schema_name || '.' || table_name)::regclass
          and
            col in 
          (
            select pg_get_indexdef(idx.indexrelid, k + 1, true)
            from generate_subscripts(idx.indkey, 1) as k
          )
   loop
     execute 'drop index ' || schema_name || '.' || idx_name;
   end loop;
end;
$$
language plpgsql;

do $$
begin
raise notice '==== Dropping road geometry indexes ===';
end
$$;

select _drop_index('tempus', 'road_node', 'geom');
select _drop_index('tempus', 'road_section', 'geom');
select _drop_index('tempus', 'road_section', 'node_from');
select _drop_index('tempus', 'road_section', 'node_to');
