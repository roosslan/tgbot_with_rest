-- Table: public.consumers

-- DROP TABLE IF EXISTS public.consumers;

CREATE TABLE IF NOT EXISTS public.consumers
(
    id text COLLATE pg_catalog."default" NOT NULL,
    username text COLLATE pg_catalog."default",
    additional text COLLATE pg_catalog."default",
    CONSTRAINT consumers_pkey PRIMARY KEY (id)
)

TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.consumers
    OWNER to chanserv;