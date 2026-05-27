-- Table: public.issues

-- DROP TABLE IF EXISTS public.issues;

CREATE TABLE IF NOT EXISTS public.issues
(
    id integer NOT NULL GENERATED ALWAYS AS IDENTITY ( INCREMENT 1 START 300 MINVALUE 1 MAXVALUE 2147483647 CACHE 1 ),
    consumer_id text COLLATE pg_catalog."default",
    consumer_name text COLLATE pg_catalog."default",
    consumer_link text COLLATE pg_catalog."default",
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
    photos text COLLATE pg_catalog."default",
    project text COLLATE pg_catalog."default",
    CONSTRAINT issues_pkey PRIMARY KEY (id)
)

TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.issues
    OWNER to chanserv;