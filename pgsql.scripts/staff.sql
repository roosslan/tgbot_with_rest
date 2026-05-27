-- Table: public.staff

-- DROP TABLE IF EXISTS public.staff;

CREATE TABLE IF NOT EXISTS public.staff
(
    id text COLLATE pg_catalog."default" NOT NULL,
    username text COLLATE pg_catalog."default",
    roles text COLLATE pg_catalog."default",
    additional text COLLATE pg_catalog."default",
    CONSTRAINT staff_pkey PRIMARY KEY (id)
)

TABLESPACE pg_default;

ALTER TABLE IF EXISTS public.staff
    OWNER to chanserv;