from os import getenv
import zou.app
import zou.app.models.project_status
from sqlalchemy import create_engine
from sqlalchemy.orm import Session
import doodle.project_status
PASS = getenv("KITSU_PASS")

def main():
    engine = create_engine("sqlite:///D:\\zou2.db", echo=True)

    with zou.app.app.app_context():
        prj : list[zou.app.models.project_status.ProjectStatus]= zou.app.models.project_status.ProjectStatus.query.all()
        with Session(engine) as session:
            doodle.project_status.ProjectStatus.metadata.create_all(engine)
            for i in prj:
                prj = doodle.project_status.ProjectStatus(
                    uuid_id=i.id,
                    name=i.name,
                    color=i.color,
                )
                session.add(prj)
            session.commit()
        print("tset lev")
        # print(prj)



if __name__ == "__main__":
    main()
