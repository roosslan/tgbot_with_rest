-- Table: public.tekla_issues

-- DROP TABLE IF EXISTS public.tekla_issues;

CREATE TABLE IF NOT EXISTS public.tekla_issues
(
    consumer_id text COLLATE pg_catalog."default",
    consumer_name text COLLATE pg_catalog."default",
    consumer_link text COLLATE pg_catalog."default",
    consumer_adname text COLLATE pg_catalog."default",
    section text COLLATE pg_catalog."default",
    category text COLLATE pg_catalog."default",
    theme text COLLATE pg_catalog."default",
    description text COLLATE pg_catalog."default",
    created_time text COLLATE pg_catalog."default",
    idcard text COLLATE pg_catalog."default",
    card_status text COLLATE pg_catalog."default",
    idcard_list text COLLATE pg_catalog."default",
    contractor_list text COLLATE pg_catalog."default",
    contractor_name text COLLATE pg_catalog."default",
    contractor_id text COLLATE pg_catalog."default",
    contractor_link text COLLATE pg_catalog."default",
    contractor_adname text COLLATE pg_catalog."default",
    photos text COLLATE pg_catalog."default",
    project text COLLATE pg_catalog."default",
    id bigint NOT NULL DEFAULT nextval('tekla_issues_id_seq'::regclass),
    CONSTRAINT tekla_issues_pkey PRIMARY KEY (id)
)

TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.tekla_issues
    OWNER to chanserv;