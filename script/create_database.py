import zou.app.models.project_status
from sqlalchemy import create_engine
from sqlalchemy.orm import Session

def main():
    engine = create_engine("sqlite:///D:\\zou.db")
    # zou.app.models.project_status.ProjectStatus.metadata.create_all(engine)
    with Session(engine) as session:
        session.add(zou.app.models.project_status.ProjectStatus(name="Open2", color="#00FF00"))
        session.commit()

if __name__ == "__main__":
    main()
