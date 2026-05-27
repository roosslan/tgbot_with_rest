-- Table: public.settings

-- DROP TABLE IF EXISTS public.settings;

CREATE TABLE IF NOT EXISTS public.settings
(
    name character varying(50) COLLATE pg_catalog."default" NOT NULL,
    value text COLLATE pg_catalog."default",
    description text COLLATE pg_catalog."default",
    CONSTRAINT settings_pkey PRIMARY KEY (name)
)

TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.settings
    OWNER to chanserv;